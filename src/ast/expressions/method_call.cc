#include "./method_call.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/qat_module.hpp"
#include "../../IR/type_id.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/struct_type.hpp"
#include "../../IR/types/vector.hpp"
#include "../prerun/method_call.hpp"

#include <llvm/IR/Intrinsics.h>

namespace qat::ast {

void MethodCall::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	UPDATE_DEPS(instance);
	for (auto arg : arguments) {
		UPDATE_DEPS(arg);
	}
	for (auto mod : ir::Mod::allModules) {
		for (auto modEnt : mod->entityEntries) {
			if (modEnt->type == ir::EntityType::defaultDoneSkill) {
				if (modEnt->has_child(memberName.value)) {
					ent->addDependency(ir::EntityDependency{modEnt, ir::DependType::partial, phase});
				}
			} else if (modEnt->type == ir::EntityType::structType) {
				if (modEnt->has_child(memberName.value)) {
					ent->addDependency(ir::EntityDependency{modEnt, ir::DependType::childrenPartial, phase});
				}
			}
		}
	}
}

String MethodQuery::kind_to_string(ir::Ctx* irCtx) const {
	if (kind == MethodHolderKind::EXPANDED) {
		return "type " + irCtx->color(((ir::ExpandedType*)holder)->to_string());
	} else {
		String prefix;
		if (kind == MethodHolderKind::DEFAULT_IMPL) {
			prefix = "default implementation";
		} else if (kind == MethodHolderKind::UNNAMED_IMPL) {
			prefix = "implementation of " + irCtx->color(((ir::DoneSkill*)holder)->get_skill()->get_full_name());
		} else if (kind == MethodHolderKind::NAMED_IMPL) {
			prefix = "implementation named " + irCtx->color(((ir::DoneSkill*)holder)->get_name().value) + " of " +
			         irCtx->color(((ir::DoneSkill*)holder)->get_skill()->get_full_name());
		}
		return prefix + " for type " + irCtx->color(((ir::DoneSkill*)holder)->get_candidate_type()->to_string()) +
		       " at " + irCtx->color(((ir::DoneSkill*)holder)->get_file_range()->to_string());
	}
}

String MethodQuery::to_disambiguity(ir::Ctx* irCtx, bool isSelfCall, String const& methodName) const {
	String res(isSelfCall ? "''" : "'");
	if (methodType == MethodQueryType::VARIATION) {
		res += "var:";
	}
	res.append(methodName).append("(...)");
	switch (kind) {
		case MethodHolderKind::DEFAULT_IMPL:
		case MethodHolderKind::EXPANDED: {
			res.append("'of:type");
			res = irCtx->color(res);
			break;
		}
		case MethodHolderKind::UNNAMED_IMPL: {
			res.append("'of:skill");
			res = irCtx->color(res);
			break;
		}
		case MethodHolderKind::NAMED_IMPL: {
			res.append("'of:" + ((ir::DoneSkill*)holder)->get_name().value);
			res = irCtx->color(res);
			res.append(" and don't forget to import the implementation " +
			           irCtx->color(((ir::DoneSkill*)holder)->get_name().value));
			break;
		}
	}
	return res;
}

String MethodQuery::to_location(ir::Ctx* irCtx, String const& methodName) const {
	return irCtx->color((methodType == MethodQueryType::VARIATION
	                         ? "var:"
	                         : (methodType == MethodQueryType::VALUED ? "self:" : "")) +
	                    methodName) +
	       " in " + kind_to_string(irCtx);
}

ir::Value* MethodCall::emit(EmitCtx* ctx) {
	SHOW("Member variable emitting")
	if (isExpSelf) {
		if (ctx->get_fn()->is_method()) {
			auto* memFn = (ir::Method*)ctx->get_fn();
			if (memFn->is_static_method()) {
				ctx->Error("This is a static method and hence cannot call method on the parent instance", fileRange);
			}
		} else {
			ctx->Error(
			    "The parent function is not a method of any type and hence cannot call methods on the parent instance",
			    fileRange);
		}
	} else {
		if (instance->nodeType() == NodeType::SELF) {
			ctx->Error(
			    "Do not use this syntax for calling methods on the parent instance. Use " +
			        ctx->color((callNature == MethodCallNature::VAR ? "''var:" : "''") + memberName.value + "(...)") +
			        " instead",
			    fileRange);
		}
	}
	auto* inst     = instance->emit(ctx);
	auto* instType = inst->get_ir_type();
	bool  isVar    = inst->has_variability();
	bool  isRef    = false;
	if (instType->is_ref()) {
		isRef = true;
		inst->load_ghost_ref(ctx->irCtx->builder);
		isVar    = instType->as_ref()->has_variability();
		instType = instType->as_ref()->get_subtype();
	} else if (inst->is_ghost_ref()) {
		isRef = true;
	}
	if (inst->is_prerun_value() && instType->is_typed()) { // TODO: Support type traits for normal expressions
		return handle_type_wrap_functions(inst->as_prerun(), arguments, memberName, ctx, fileRange);
	} else if (instType->is_vector()) {
		auto* vecTy = instType->as_vector();
		if (memberName.value == "insert") {
			if (not vecTy->is_scalable()) {
				ctx->Error(
				    "Method " + ctx->color("insert") +
				        " can only be called on scalable vectors. This expression is of type " +
				        ctx->color(vecTy->to_string()) +
				        " which is not a scalable vector type. Scalable version of this vector type will look like " +
				        ctx->color("vec:[?, " + vecTy->get_element_type()->to_string() + "," +
				                   std::to_string(vecTy->get_count()) + "]"),
				    fileRange);
			}
			if (isRef) {
				ctx->Error(
				    "The " + ctx->color("insert") +
				        " method works on scalable vector values instead of references. The expression on which the method is called, is " +
				        (inst->get_ir_type()->is_ref() ? "a reference" : "reference-like") + ". Use " +
				        ctx->color("'copy") + " or " + ctx->color("'move") + " or " + ctx->color("'swap(value)") +
				        " accordingly to get the scalable vector value, before calling this method.",
				    fileRange);
			}
			if (arguments.size() != 1u) {
				ctx->Error("Method " + ctx->color(memberName.value) + " requires " +
				               (arguments.size() > 1 ? "only " : "") + "one argument of type " +
				               ctx->color(vecTy->get_non_scalable_type(ctx->irCtx)->to_string()),
				           fileRange);
			}
			if (arguments[0]->has_type_inferrance()) {
				arguments[0]->as_type_inferrable()->set_inference_type(vecTy->get_non_scalable_type(ctx->irCtx));
			}
			auto* argVec = arguments[0]->emit(ctx);
			argVec = ir::Logic::handle_pass_semantics(ctx, argVec->get_pass_type(), argVec, arguments[0]->fileRange);
			if (not argVec->get_ir_type()->is_same(vecTy->get_non_scalable_type(ctx->irCtx))) {
				ctx->Error("The argument provided to " + ctx->color(memberName.value) + " is expected to be of type " +
				               ctx->color(vecTy->get_non_scalable_type(ctx->irCtx)->to_string()),
				           fileRange);
			}
			auto result = ctx->irCtx->builder.CreateInsertVector(
			    vecTy->get_llvm_type(), inst->get_llvm(), argVec->get_llvm(),
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u));
			return ir::Value::get(result, vecTy, false);
		} else if (memberName.value == "reverse") {
			if (isRef) {
				ctx->Error(
				    "The " + ctx->color("reverse") +
				        " method works on vector values instead of references. The expression on which the method is called, is " +
				        (inst->get_ir_type()->is_ref() ? "a reference" : "reference-like") + ". Use " +
				        ctx->color("'copy") + " or " + ctx->color("'move") + " or " + ctx->color("'swap(value)") +
				        " accordingly to get the vector value, before caling this method..",
				    fileRange);
			}
			if (not arguments.empty()) {
				ctx->Error("The method " + ctx->color("reverse") + " expects zero arguments, but " +
				               ctx->color(std::to_string(arguments.size())) + " value" +
				               (arguments.size() > 1 ? "s" : "") + " were provided",
				           fileRange);
			}
			return ir::Value::get(ctx->irCtx->builder.CreateVectorReverse(inst->get_llvm()), vecTy, false);
		}
	}
	auto             modSkills       = ctx->mod->get_skills_in_module();
	auto             broughtSkills   = ctx->mod->get_all_brought_skills();
	bool             hasVariation    = false;
	bool             hasNormalMethod = false;
	bool             hasValuedMethod = false;
	ir::Method*      variationMethod = nullptr;
	ir::Method*      normalMethod    = nullptr;
	ir::Method*      valueMethod     = nullptr;
	Vec<MethodQuery> varHolders;
	Vec<MethodQuery> normalHolders;
	Vec<MethodQuery> valueMethodHolders;
	auto             foundLocationsMessage = [&]() {
        if (varHolders.empty() && normalHolders.empty() && valueMethodHolders.empty()) {
            return String();
        }
        String res("\nFound methods named " + memberName.value + " in the following locations:\n");
        u16    index = 1;
        for (auto& it : normalHolders) {
            res.append("=> ").append(it.to_location(ctx->irCtx, memberName.value)).append("\n");
            index++;
        }
        for (auto& it : varHolders) {
            res.append("=> ").append(it.to_location(ctx->irCtx, memberName.value)).append("\n");
            index++;
        }
        for (auto& it : valueMethodHolders) {
            res.append("=> ").append(it.to_location(ctx->irCtx, memberName.value)).append("\n");
            index++;
        }
        return res;
	};
	auto potentialLocationsMessage = [&]() {
		if (normalHolders.empty() && varHolders.empty() && valueMethodHolders.empty() &&
		    instType->has_skills_for_method_name(memberName.value)) {
			String res("\nFound the following skills containing a method named " + ctx->color(memberName.value) +
			           ".\n Check to see if any of these skills contain the method you are trying to call,"
			           "\n and import the appropriate skill into the current module to use that method\n");
			auto   skills = instType->get_skills_for_method_name(memberName.value);
			for (auto it = skills.first; it != skills.second; it++) {
				const auto skill = (*it).second;
				res += "=> " + ctx->color(skill->get_full_name()) +
				       ((skill->get_name().range != nullptr) ? (" at " + skill->get_name().range->to_string() + "\n")
				                                             : "\n");
			}
			return res;
		}
		return String();
	};
	auto multipleMethodsFound = [&](Vec<MethodQuery> const& list, String methodType) {
		auto res =
		    "Multiple " + methodType + " have been found with the name " + ctx->color(memberName.value) +
		    " for the type " + ctx->color(instType->to_string()) +
		    ". The compiler cannot arbitrarily choose one of these methods to call, as that will lead to unpredictable behaviour."
		    " The methods found are as follows, followed by what you can do to choose the specific method (disambiguity):";
		for (auto& it : list) {
			res.append("\n=>")
			    .append(it.to_location(ctx->irCtx, memberName.value))
			    .append(" can be chosen using ")
			    .append(it.to_disambiguity(ctx->irCtx, isExpSelf, memberName.value));
		}
	};
	if (disambiguity == MethodDisambiguity::DONE_SKILL) {
		auto access   = ctx->get_access_info();
		auto doneName = doneSkill.value();
		if (ctx->mod->has_named_implementation(doneName.value, access) or
		    ctx->mod->has_brought_named_implementation(doneName.value, access) or
		    ctx->mod->has_named_implementation_in_imports(doneName.value, access).first) {
			auto doneSk = ctx->mod->get_named_implementation(doneName.value, access);
			if (doneSk->has_variation_method(memberName.value)) {
				varHolders.push_back(MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::NAMED_IMPL, doneSk});
				hasVariation    = true;
				variationMethod = doneSk->get_variation_method(memberName.value);
			}
			if (doneSk->has_normal_method(memberName.value)) {
				varHolders.push_back(MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::NAMED_IMPL, doneSk});
				hasNormalMethod = true;
				normalMethod    = doneSk->get_normal_method(memberName.value);
			}
			if (doneSk->has_valued_method(memberName.value)) {
				varHolders.push_back(MethodQuery{MethodQueryType::VALUED, MethodHolderKind::NAMED_IMPL, doneSk});
				hasValuedMethod = true;
				valueMethod     = doneSk->get_valued_method(memberName.value);
			}
		} else {
			ctx->Error(
			    "The disambiguity specification provides the named implementation " + ctx->color(doneName.value) +
			        ", but could not find a implementation with that name in the current module or its imported modules",
			    doneName.range);
		}
	} else {
		if (instType->is_expanded() and
		    (disambiguity == MethodDisambiguity::TYPE or disambiguity == MethodDisambiguity::NONE)) {
			auto expTy = instType->as_expanded();
			if (expTy->has_variation(memberName.value)) {
				varHolders.push_back(MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::EXPANDED, expTy});
				hasVariation    = true;
				variationMethod = expTy->get_variation(memberName.value);
			}
			if (expTy->has_normal_method(memberName.value)) {
				normalHolders.push_back(MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::EXPANDED, expTy});
				hasNormalMethod = true;
				normalMethod    = expTy->get_normal_method(memberName.value);
			}
			if (expTy->has_valued_method(memberName.value)) {
				valueMethodHolders.push_back(MethodQuery{MethodQueryType::VALUED, MethodHolderKind::EXPANDED, expTy});
				hasValuedMethod = true;
				valueMethod     = expTy->get_valued_method(memberName.value);
			}
		}
		if (disambiguity == MethodDisambiguity::TYPE or disambiguity == MethodDisambiguity::NONE) {
			for (auto* doneSk : instType->get_default_implementations()) {
				if (doneSk->has_variation_method(memberName.value)) {
					varHolders.push_back(
					    MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::DEFAULT_IMPL, doneSk});
					hasVariation    = true;
					variationMethod = doneSk->get_variation_method(memberName.value);
				}
				if (doneSk->has_normal_method(memberName.value)) {
					normalHolders.push_back(
					    MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::DEFAULT_IMPL, doneSk});
					hasNormalMethod = true;
					normalMethod    = doneSk->get_normal_method(memberName.value);
				}
				if (doneSk->has_valued_method(memberName.value)) {
					valueMethodHolders.push_back(
					    MethodQuery{MethodQueryType::VALUED, MethodHolderKind::DEFAULT_IMPL, doneSk});
					hasValuedMethod = true;
					valueMethod     = doneSk->get_valued_method(memberName.value);
				}
			}
		}
		for (auto* sk : modSkills) {
			if ((disambiguity == MethodDisambiguity::SKILL or disambiguity == MethodDisambiguity::NONE) and
			    instType->has_unnamed_implementation_for(sk)) {
				auto* const doneSk = instType->get_unnamed_implementation_for(sk);
				if (doneSk->has_variation_method(memberName.value)) {
					varHolders.push_back(
					    MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasVariation    = true;
					variationMethod = doneSk->get_variation_method(memberName.value);
				}
				if (doneSk->has_normal_method(memberName.value)) {
					normalHolders.push_back(
					    MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasNormalMethod = true;
					normalMethod    = doneSk->get_normal_method(memberName.value);
				}
				if (doneSk->has_valued_method(memberName.value)) {
					valueMethodHolders.push_back(
					    MethodQuery{MethodQueryType::VALUED, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasValuedMethod = true;
					valueMethod     = doneSk->get_valued_method(memberName.value);
				}
			}
			if ((disambiguity == MethodDisambiguity::NONE) and instType->has_named_implementation_for(sk)) {
				auto doneSkills = instType->get_named_implementations_for(sk);
				for (auto it = doneSkills.first; it != doneSkills.second; it++) {
					auto doneSk = (*it).second;
					if (doneSk->has_variation_method(memberName.value)) {
						varHolders.push_back(
						    MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::NAMED_IMPL, doneSk});
						hasVariation    = true;
						variationMethod = doneSk->get_variation_method(memberName.value);
					}
					if (doneSk->has_normal_method(memberName.value)) {
						normalHolders.push_back(
						    MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::NAMED_IMPL, doneSk});
						hasNormalMethod = true;
						normalMethod    = doneSk->get_normal_method(memberName.value);
					}
					if (doneSk->has_valued_method(memberName.value)) {
						valueMethodHolders.push_back(
						    MethodQuery{MethodQueryType::VALUED, MethodHolderKind::NAMED_IMPL, doneSk});
						hasValuedMethod = true;
						valueMethod     = doneSk->get_valued_method(memberName.value);
					}
				}
			}
		}
		for (auto& brought : broughtSkills) {
			auto sk = brought.get();
			if ((disambiguity == MethodDisambiguity::SKILL or disambiguity == MethodDisambiguity::NONE) and
			    instType->has_unnamed_implementation_for(sk)) {
				auto* const doneSk = instType->get_unnamed_implementation_for(sk);
				if (doneSk->has_variation_method(memberName.value)) {
					varHolders.push_back(
					    MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasVariation    = true;
					variationMethod = doneSk->get_variation_method(memberName.value);
				}
				if (doneSk->has_normal_method(memberName.value)) {
					normalHolders.push_back(
					    MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasNormalMethod = true;
					normalMethod    = doneSk->get_normal_method(memberName.value);
				}
				if (doneSk->has_valued_method(memberName.value)) {
					valueMethodHolders.push_back(
					    MethodQuery{MethodQueryType::VALUED, MethodHolderKind::UNNAMED_IMPL, doneSk});
					hasValuedMethod = true;
					valueMethod     = doneSk->get_valued_method(memberName.value);
				}
			}
			if ((disambiguity == MethodDisambiguity::NONE) and instType->has_named_implementation_for(sk)) {
				auto doneSkills = instType->get_named_implementations_for(sk);
				for (auto it = doneSkills.first; it != doneSkills.second; it++) {
					auto doneSk = (*it).second;
					if (doneSk->has_variation_method(memberName.value)) {
						varHolders.push_back(
						    MethodQuery{MethodQueryType::VARIATION, MethodHolderKind::NAMED_IMPL, doneSk});
						hasVariation    = true;
						variationMethod = doneSk->get_variation_method(memberName.value);
					}
					if (doneSk->has_normal_method(memberName.value)) {
						normalHolders.push_back(
						    MethodQuery{MethodQueryType::NORMAL, MethodHolderKind::NAMED_IMPL, doneSk});
						hasNormalMethod = true;
						normalMethod    = doneSk->get_normal_method(memberName.value);
					}
					if (doneSk->has_valued_method(memberName.value)) {
						valueMethodHolders.push_back(
						    MethodQuery{MethodQueryType::VALUED, MethodHolderKind::NAMED_IMPL, doneSk});
						hasValuedMethod = true;
						valueMethod     = doneSk->get_valued_method(memberName.value);
					}
				}
			}
		}
	}
	ir::Method* usableFn = nullptr;
	if (callNature == MethodCallNature::VAR) {
		if (not isRef) {
			ctx->Error(
			    "Found a value expression and a variation-method cannot be called on it."
			    " Only value-methods can be called on such expressions. Variation & normal"
			    " methods require the expression to either be a reference or be reference-like (let-bindings, globals).",
			    instance->fileRange);
		}
		if (not isVar) {
			ctx->Error("Got an expression of type " + ctx->color(inst->get_ir_type()->to_string()) + ", which is a " +
			               (inst->get_ir_type()->is_ref()
			                    ? "reference without variability"
			                    : (inst->is_ghost_ref() ? "reference-like expression without variability" : "value")) +
			               ", so a variation method cannot be called on it",
			           instance->fileRange);
		}
		if (not hasVariation) {
			if (hasNormalMethod or hasValuedMethod) {
				ctx->Error(
				    (hasNormalMethod
				         ? ("Found a normal-method named " + ctx->color(memberName.value) + " for the type " +
				            ctx->color(instType->to_string()) +
				            ", but could not find a variation-method with the same name. If you indeed meant to call the normal-method, remove the " +
				            ctx->color("var:") + " qualifier from the method call and use the syntax " +
				            ctx->color(String(isExpSelf ? "''" : "'") + memberName.value + "(...)") + " instead" +
				            (hasValuedMethod ? "\n" : ""))
				         : "") +
				        (hasValuedMethod
				             ? ("Found a value-method named " + ctx->color(memberName.value) + " for the type " +
				                ctx->color(instType->to_string()) +
				                ", but could not find a variation-method with the same name. The value-method cannot be called because a " +
				                (inst->get_ir_type()->is_ref() ? "reference" : "reference-like") +
				                " expression was given to call the method on. Value-methods can only be called on values. If you want to convert this " +
				                (inst->get_ir_type()->is_ref() ? "reference" : "reference-like") +
				                " expression to a value, use " + ctx->color(isExpSelf ? "''copy" : "'copy") + " or " +
				                ctx->color(isExpSelf ? "''move" : "'move") + " or " +
				                ctx->color(isExpSelf ? "''swap(value)" : "'swap(value)") +
				                " accordingly, before the method call. And don't forget to remove the " +
				                ctx->color("var:") + " qualifier from the method call")
				             : "") +
				        foundLocationsMessage(),
				    fileRange);
			} else {
				ctx->Error(
				    "The expression on which the method is meant to be called, is " +
				        String(inst->get_ir_type()->is_ref() ? "a reference" : "reference-like") + ", and the " +
				        ctx->color("var:") +
				        " qualifier was provided for the method call, so variation-methods that matched the name " +
				        ctx->color(memberName.value) +
				        " were searched for. No variation-method with that name could be found for type " +
				        ctx->color(instType->to_string()) + foundLocationsMessage() + potentialLocationsMessage(),
				    fileRange);
			}
		}
		if (varHolders.size() > 1) {
			multipleMethodsFound(varHolders, "variation-methods");
		}
		usableFn = variationMethod;
	} else {
		if (isRef) {
			if (not hasNormalMethod) {
				if (hasVariation or hasValuedMethod) {
					ctx->Error(
					    (hasVariation
					         ? ("Found a variation-method named " + ctx->color(memberName.value) + " for the type " +
					            ctx->color(instType->to_string()) +
					            ", but could not find a normal method with the same name. If you indeed meant to call the variation-method, use the syntax " +
					            ctx->color((isExpSelf ? "''var:" : "'var:") + memberName.value + "(...)") + " instead" +
					            (hasValuedMethod ? "\n" : ""))
					         : "") +
					        (hasValuedMethod
					             ? ("Found a value-method named " + ctx->color(memberName.value) + " for the type " +
					                ctx->color(instType->to_string()) +
					                ", but could not find a normal-method with the same name. The value-method cannot be called because a " +
					                (inst->get_ir_type()->is_ref() ? "reference" : "reference-like") +
					                " expression was given to call the method on. Value-methods can only be called on values. If you want to convert this " +
					                (inst->get_ir_type()->is_ref() ? "reference" : "reference-like") +
					                " expression to a value, use " + ctx->color(isExpSelf ? "''copy" : "'copy") +
					                " or " + ctx->color(isExpSelf ? "''move" : "'move") + " or " +
					                ctx->color(isExpSelf ? "''swap(value)" : "'swap(value)") +
					                " accordingly, before the method call.")
					             : "") +
					        foundLocationsMessage(),
					    fileRange);
				} else {
					ctx->Error(
					    "The expression on which the method is meant to be called, is " +
					        String(inst->get_ir_type()->is_ref() ? "a reference" : "reference-like") +
					        ", so normal-methods that matched the name " + ctx->color(memberName.value) +
					        " were searched for (variation-methods were skipped as there is no " + ctx->color("var:") +
					        " qualifier for the method call). No normal-method with that name could be found for type " +
					        ctx->color(instType->to_string()) + foundLocationsMessage() + potentialLocationsMessage(),
					    fileRange);
				}
			}
			if (normalHolders.size() > 1) {
				multipleMethodsFound(normalHolders, "normal-methods");
			}
			usableFn = normalMethod;
		} else {
			if (not hasValuedMethod) {
				if (hasNormalMethod or hasVariation) {
					ctx->Error(
					    (hasNormalMethod
					         ? ("Found a normal-method named " + ctx->color(memberName.value) + " for the type " +
					            ctx->color(instType->to_string()) +
					            ", but could not find a value-method with the same name. The expression on which the method is meant to"
					            " be called, is a value, so a normal-method cannot be called on it. If you want to call the normal-method,"
					            " convert the value to a reference-like expression using a let-binding. You can either create a separate"
					            " let-binding, or you can create a let-binding inline using " +
					            ctx->color("'let") + (hasVariation ? "\n" : ""))
					         : "") +
					        (hasVariation
					             ? ("Found a variation-method named " + ctx->color(memberName.value) +
					                " for the type " + ctx->color(instType->to_string()) +
					                ", but could not find a value-method with the same name. The expression on which the method is meant to"
					                " be called, is a value, so a variation-method cannot be called on it. If you want to call the variation-method,"
					                " convert the value to a reference-like expression using a let-binding. You can either create a separate"
					                " let-binding, or you can create let-binding inline using " +
					                ctx->color("'let") + ". Don't forget to provide the " + ctx->color("var:") +
					                " qualifier for the method call after that to call the variation-method.")
					             : "") +
					        foundLocationsMessage(),
					    fileRange);
				} else {
					ctx->Error("The expression on which the method is meant to called, is a value,", fileRange);
				}
			}
			if (valueMethodHolders.size() > 1) {
				multipleMethodsFound(valueMethodHolders, "value-methods");
			}
			usableFn = valueMethod;
		}
	}
	if (not usableFn->is_accessible(ctx->get_access_info())) {
		ctx->Error("Method " + ctx->color(memberName.value) + " for type " + ctx->color(instType->to_string()) +
		               " is not accessible here, and hence cannot be called",
		           fileRange);
	}
	if (isExpSelf && instType->is_struct() && ctx->get_fn()->is_method()) {
		auto thisFn = (ir::Method*)ctx->get_fn();
		if (thisFn->is_constructor()) {
			Vec<String> missingMembers;
			for (usize i = 0; i < instType->as_struct()->get_field_count(); i++) {
				if (not thisFn->is_member_initted(i)) {
					missingMembers.push_back(instType->as_struct()->get_field_name_at(i));
				}
			}
			if (not missingMembers.empty()) {
				String message;
				for (usize i = 0; i < missingMembers.size(); i++) {
					message.append(ctx->color(missingMembers[i]));
					if (i == missingMembers.size() - 2) {
						message.append(" and ");
					} else if (i + 1 < missingMembers.size()) {
						message.append(", ");
					}
				}
				// NOTE - Maybe consider changing this to deeper call-tree-analysis
				// which might be impossible because of skills and implementations which are invisible at this point
				ctx->Error("Cannot call the " +
				               String((callNature == MethodCallNature::VAR) ? "variation-" : "normal-") +
				               "method as member field" + (missingMembers.size() > 1 ? "s " : " ") + message +
				               " of this type have not been initialised yet. If the field" +
				               (missingMembers.size() > 1 ? "s or their" : " or its") +
				               " type have a default value, it will be used for initialisation only"
				               " at the end of this constructor. Use the syntax " +
				               ctx->color("''fieldName := value.") + " to initialise the missing field" +
				               (missingMembers.size() > 1 ? "s" : "") + ", and make sure that " +
				               (missingMembers.size() > 1 ? "they are" : "it is") +
				               " initialised as early as possible, before calling any methods.",
				           fileRange);
			}
		}
		thisFn->add_method_call(usableFn);
	}
	//
	const auto fnArgsTy      = usableFn->get_ir_type()->as_function()->get_argument_types();
	const auto isVariadicArg = usableFn->get_ir_type()->as_function()->is_variadic();
	if (isVariadicArg) {
		if ((fnArgsTy.size() - 1) > arguments.size()) {
			ctx->Error("This method is a variadic function and requires at least " +
			               ctx->color(std::to_string(fnArgsTy.size() - 1)) + " arguments to be provided",
			           fileRange);
		}
	} else if ((fnArgsTy.size() - 1) != arguments.size()) {
		ctx->Error("Number of arguments provided for the method call does not match the signature", fileRange);
	}
	Vec<llvm::Value*> argVals;
	auto              localID = inst->get_local_id();
	argVals.push_back(inst->get_llvm());
	Vec<ir::Value*> varArgs;
	const auto*     fnTy         = usableFn->get_ir_type()->as_function();
	const auto      fnTyArgCount = fnTy->get_argument_count();
	for (usize i = 0; i < arguments.size(); i++) {
		const auto j = i + 1;
		if (j < fnTyArgCount) {
			auto* argTy = fnTy->get_argument_type_at(i + 1)->get_type();
			if (arguments[i]->has_type_inferrance()) {
				arguments[i]->as_type_inferrable()->set_inference_type(argTy);
			}
		} else if (arguments[i]->has_type_inferrance() and usableFn->get_variadics().kind == ir::VariadicsKind::TYPED) {
			arguments[i]->as_type_inferrable()->set_inference_type(usableFn->get_variadics().type);
		}
		auto argVal = arguments[i]->emit(ctx);
		argVal      = ir::Logic::handle_pass_semantics(ctx, argVal->get_pass_type(), argVal, arguments[i]->fileRange);
		if (j < fnTyArgCount) {
			if (not fnArgsTy[j]->get_type()->is_compatible_with(argVal->get_ir_type())) {
				ctx->Error("The argument " + (fnArgsTy[j]->get_name()) + " has the type " +
				               ctx->color(fnArgsTy[j]->get_type()->to_string()) + ", but got an expression of type " +
				               ctx->color(argVal->get_ir_type()->to_string()) + " instead",
				           arguments[i]->fileRange);
			}
			argVals.push_back(argVal->get_llvm());
		} else {
			// Argument is variadic
			switch (usableFn->get_variadics().kind) {
				case ir::VariadicsKind::LEGACY: {
					if (not(argVal->get_ir_type()->has_simple_copy() && argVal->get_ir_type()->has_simple_move())) {
						ctx->Error(
						    "This expression is passed to the method as a variadic argument, and the method has legacy variadics. "
						    "This means that only types with simple-copy and simple-move are allowed to passed as variadic arguments. "
						    "Found an expression of type " +
						        ctx->color(argVal->get_ir_type()->to_string()) + " instead.",
						    arguments[i]->fileRange);
					}
					break;
				}
				case ir::VariadicsKind::TYPED: {
					const auto varTy = usableFn->get_variadics().type;
					if (not varTy->is_compatible_with(argVal->get_ir_type())) {
						ctx->Error(
						    "This expression is passed to the method as a variadic argument, and the method has typed variadics. "
						    "Variadic arguments passed to this method is required to be of type " +
						        ctx->color(varTy->to_string()) + ", but found an expression of type " +
						        ctx->color(argVal->get_ir_type()->to_string()) + " instead",
						    arguments[i]->fileRange);
					}
					break;
				}
				default: {
				}
			}
			varArgs.push_back(argVal);
		}
	}
	if (isVariadicArg) {
		auto variadics = usableFn->get_variadics();
		switch (variadics.kind) {
			case ir::VariadicsKind::NORMAL: {
				argVals.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(ctx->irCtx->llctx), varArgs.size()));
				for (auto it : varArgs) {
					const auto tyInfo = ir::TypeInfo::create(ctx->irCtx, it->get_ir_type(), ctx->mod);
					argVals.push_back(tyInfo->id);
					argVals.push_back(it->get_llvm());
				}
				break;
			}
			case ir::VariadicsKind::LEGACY: {
				for (auto it : varArgs) {
					argVals.push_back(it->get_llvm());
				}
				break;
			}
			case ir::VariadicsKind::TYPED: {
				argVals.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(ctx->irCtx->llctx), varArgs.size()));
				for (auto it : varArgs) {
					argVals.push_back(it->get_llvm());
				}
				break;
			}
		}
	}
	return usableFn->call(ctx->irCtx, argVals, localID, ctx->mod);
}

Json MethodCall::to_json() const {
	Vec<JsonValue> argsJSON;
	for (auto* arg : arguments) {
		argsJSON.emplace_back(arg->to_json());
	}
	return Json()
	    ._("nodeType", "memberFunctionCall")
	    ._("instance", instance->to_json())
	    ._("function", memberName)
	    ._("arguments", argsJSON)
	    ._("callNature", callNature == MethodCallNature::VAR ? "var" : "none")
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
