#include "./method_call.hpp"
#include "../../IR/qat_module.hpp"
#include "../../IR/types/vector.hpp"
#include "../prerun/method_call.hpp"

#include <llvm/IR/Intrinsics.h>

namespace qat::ast {

void MethodCall::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                     EmitCtx* ctx) {
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
			ctx->Error("Do not use this syntax for calling methods. Use " +
			               ctx->color("''" + memberName.value + "(...)") + " instead",
			           fileRange);
		}
	}
	auto* inst     = instance->emit(ctx);
	auto* instType = inst->get_ir_type();
	bool  isVar    = inst->is_variable();
	if (instType->is_reference()) {
		inst->load_ghost_reference(ctx->irCtx->builder);
		isVar    = instType->as_reference()->isSubtypeVariable();
		instType = instType->as_reference()->get_subtype();
	}
	if (instType->is_mark()) {
		ctx->Error("The expression is of pointer type. Please dereference the pointer to call the method",
		           instance->fileRange);
	}
	if (instType->is_expanded()) {
		if (memberName.value == "end") {
			if (isExpSelf) {
				ctx->Error("Cannot call the destructor on the parent instance. This is not allowed", fileRange);
			}
			if (!instType->as_expanded()->has_destructor()) {
				ctx->Error("Type " + ctx->color(instType->as_expanded()->get_full_name()) +
				               " does not have a destructor",
				           fileRange);
			}
			auto* desFn = instType->as_expanded()->get_destructor();
			if (!inst->is_ghost_reference() && !inst->is_reference()) {
				inst = inst->make_local(ctx, None, instance->fileRange);
			} else if (inst->is_reference()) {
				inst->load_ghost_reference(ctx->irCtx->builder);
			}
			return desFn->call(ctx->irCtx, {inst->get_llvm()}, None, ctx->mod);
		}
		auto* eTy = instType->as_expanded();
		if (callNature.has_value()) {
			if (callNature.value()) {
				if (!eTy->has_variation(memberName.value)) {
					if (eTy->has_normal_method(memberName.value)) {
						ctx->Error(ctx->color(memberName.value) + " is not a variation method of type " +
						               ctx->color(eTy->get_full_name()) + " and hence cannot be called as a variation",
						           fileRange);
					} else {
						ctx->Error("No variation method named " + ctx->color(memberName.value) + " found in type " +
						               ctx->color(eTy->get_full_name()),
						           fileRange);
					}
				}
				if (!isVar) {
					ctx->Error("The expression does not have variability and hence variation methods cannot be called",
					           instance->fileRange);
				}
			} else {
				if (!eTy->has_normal_method(memberName.value)) {
					if (eTy->has_variation(memberName.value)) {
						ctx->Error(ctx->color(memberName.value) + " is a variation method of type " +
						               ctx->color(eTy->get_full_name()) +
						               " and hence cannot be called as a normal method",
						           fileRange);
					} else {
						ctx->Error("No normal method named " + ctx->color(memberName.value) + " found in type " +
						               ctx->color(eTy->get_full_name()),
						           fileRange);
					}
				}
			}
		} else {
			if (isVar) {
				if (!eTy->has_valued_method(memberName.value) && !eTy->has_normal_method(memberName.value) &&
				    !eTy->has_variation(memberName.value)) {
					ctx->Error("Type " + ctx->color(instType->as_expanded()->to_string()) +
					               " does not have a method named " + ctx->color(memberName.value) +
					               ". Please check the logic",
					           fileRange);
				}
			} else {
				if (!eTy->has_valued_method(memberName.value)) {
					if (!eTy->has_normal_method(memberName.value)) {
						if (eTy->has_variation(memberName.value)) {
							ctx->Error(ctx->color(memberName.value) + " is a variation method of type " +
							               ctx->color(eTy->get_full_name()) +
							               " and cannot be called here as this expression " +
							               (inst->is_reference() ? "is a reference without variability"
							                                     : "does not have variability"),
							           memberName.range);
						} else {
							ctx->Error("Type " + ctx->color(instType->as_expanded()->to_string()) +
							               " does not have a method named " + ctx->color(memberName.value) +
							               ". Please check the logic",
							           fileRange);
						}
					}
				}
			}
		}
		auto* memFn =
		    (((callNature.has_value() && callNature.value()) || isVar) && eTy->has_variation(memberName.value))
		        ? eTy->get_variation(memberName.value)
		        : (eTy->has_valued_method(memberName.value) ? eTy->get_valued_method(memberName.value)
		                                                    : eTy->get_normal_method(memberName.value));
		if (!memFn->is_accessible(ctx->get_access_info())) {
			ctx->Error("Method " + ctx->color(memberName.value) + " of type " + ctx->color(eTy->get_full_name()) +
			               " is not accessible here",
			           fileRange);
		}
		if (isExpSelf && instType->is_struct() && ctx->get_fn()->is_method()) {
			auto thisFn = (ir::Method*)ctx->get_fn();
			if (thisFn->isConstructor()) {
				Vec<String> missingMembers;
				for (usize i = 0; i < instType->as_struct()->get_field_count(); i++) {
					if (!thisFn->is_member_initted(i)) {
						missingMembers.push_back(instType->as_struct()->get_field_name_at(i));
					}
				}
				if (!missingMembers.empty()) {
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
					ctx->Error("Cannot call the " + String(callNature ? "variation " : "") + "method as member field" +
					               (missingMembers.size() > 1 ? "s " : " ") + message +
					               " of this type have not been initialised yet. If the field" +
					               (missingMembers.size() > 1 ? "s or their" : " or its") +
					               " type have a default value, it will be used for initialisation only"
					               " at the end of this constructor",
					           fileRange);
				}
			}
			thisFn->add_method_call(memFn);
		}
		if (!inst->is_ghost_reference() && !inst->is_reference() &&
		    (memFn->get_method_type() != ir::MethodType::valueMethod)) {
			inst = inst->make_local(ctx, None, instance->fileRange);
		} else if ((memFn->get_method_type() == ir::MethodType::valueMethod) &&
		           (inst->is_ghost_reference() || inst->is_reference())) {
			if (instType->is_trivially_copyable() || instType->is_trivially_movable()) {
				if (inst->is_reference()) {
					inst->load_ghost_reference(ctx->irCtx->builder);
				}
				auto origInst = inst;
				inst = ir::Value::get(ctx->irCtx->builder.CreateLoad(instType->get_llvm_type(), inst->get_llvm()),
				                      instType, true);
				if (!instType->is_trivially_copyable()) {
					if (inst->is_reference() && !inst->get_ir_type()->as_reference()->isSubtypeVariable()) {
						ctx->Error("This is a reference without variability and hence cannot be trivially moved from",
						           instance->fileRange);
					} else if (inst->is_ghost_reference() && !inst->is_variable()) {
						ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
						           instance->fileRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(instType->get_llvm_type()),
					                                origInst->get_llvm());
				}
			} else {
				ctx->Error(
				    "The method to be called is a value method, so it requires a value of the parent type. The parent type is " +
				        ctx->color(instType->to_string()) + " which is not trivially copyable or movable. Please use " +
				        ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
				    fileRange);
			}
		}
		//
		auto       fnArgsTy      = memFn->get_ir_type()->as_function()->get_argument_types();
		const auto isVariadicArg = memFn->get_ir_type()->as_function()->is_variadic();
		if (isVariadicArg) {
			if ((fnArgsTy.size() - 1) > arguments.size()) {
				ctx->Error("This method is a variadic function and requires at least " +
				               ctx->color(std::to_string(fnArgsTy.size() - 1)) + " arguments to be provided",
				           fileRange);
			}
		} else if ((fnArgsTy.size() - 1) != arguments.size()) {
			ctx->Error("Number of arguments provided for the method call does not match the signature", fileRange);
		}
		Vec<ir::Value*> argsEmit;
		auto*           fnTy         = memFn->get_ir_type()->as_function();
		auto            fnTyArgCount = fnTy->get_argument_count();
		for (usize i = 0; i < arguments.size(); i++) {
			if ((i + 1) < fnTyArgCount) {
				auto* argTy = fnTy->get_argument_type_at(i + 1)->get_type();
				if (arguments[i]->has_type_inferrance()) {
					arguments[i]->as_type_inferrable()->set_inference_type(argTy);
				}
			}
			argsEmit.push_back(arguments[i]->emit(ctx));
		}
		SHOW("Argument values generated for method")
		for (usize i = 1; i < fnTy->get_argument_count(); i++) {
			auto fnArgType = fnArgsTy[i]->get_type();
			auto argType   = argsEmit[i - 1]->get_ir_type();
			if (!(fnArgType->is_same(argType) || fnArgType->isCompatible(argType) ||
			      (fnArgType->is_reference() && argsEmit[i - 1]->is_ghost_reference() &&
			       fnArgType->as_reference()->get_subtype()->is_same(argType)) ||
			      (argType->is_reference() && argType->as_reference()->get_subtype()->is_same(fnArgType)))) {
				ctx->Error("Type of this expression " + ctx->color(argType->to_string()) +
				               " does not match the type of the "
				               "corresponding argument " +
				               ctx->color(memFn->get_ir_type()->as_function()->get_argument_type_at(i)->get_name()) +
				               " of the function " + ctx->color(memFn->get_full_name()),
				           arguments.at(i - 1)->fileRange);
			}
		} //
		Vec<llvm::Value*> argVals;
		auto              localID = inst->get_local_id();
		argVals.push_back(inst->get_llvm());
		for (usize i = 1; i < fnTy->get_argument_count(); i++) {
			auto* currArg = argsEmit[i - 1];
			if (fnArgsTy[i]->get_type()->is_reference()) {
				auto fnRefTy = fnArgsTy[i]->get_type()->as_reference();
				if (currArg->is_reference()) {
					if (fnRefTy->isSubtypeVariable() &&
					    not currArg->get_ir_type()->as_reference()->isSubtypeVariable()) {
						ctx->Error("The type of the argument is " + ctx->color(fnArgsTy[i]->get_type()->to_string()) +
						               " but the expression is of type " +
						               ctx->color(currArg->get_ir_type()->to_string()),
						           arguments[i - 1]->fileRange);
					}
					currArg->load_ghost_reference(ctx->irCtx->builder);
				} else if (not currArg->is_ghost_reference()) {
					ctx->Error("Cannot pass a value for the argument that expects a reference",
					           arguments[i - 1]->fileRange);
				} else if (fnArgsTy[i]->get_type()->as_reference()->isSubtypeVariable()) {
					if (not currArg->is_variable()) {
						ctx->Error(
						    "The argument " + ctx->color(fnArgsTy.at(i)->get_name()) + " is of type " +
						        ctx->color(fnArgsTy[i]->get_type()->to_string()) +
						        " which is a reference with variability, but this expression does not have variability",
						    arguments[i - 1]->fileRange);
					}
				}
			} else if (currArg->is_reference() || currArg->is_ghost_reference()) {
				if (currArg->is_reference()) {
					currArg->load_ghost_reference(ctx->irCtx->builder);
				}
				SHOW("Loading ref arg at " << i - 1 << " with type " << currArg->get_ir_type()->to_string())
				auto* argTy    = fnArgsTy[i]->get_type();
				auto* argValTy = currArg->get_ir_type();
				auto  isRefVar = currArg->is_reference() ? currArg->get_ir_type()->as_reference()->isSubtypeVariable()
				                                         : currArg->is_variable();
				if (argTy->is_trivially_copyable() || argTy->is_trivially_movable()) {
					auto* argEmitLLVMValue = currArg->get_llvm();
					argsEmit[i - 1]        = ir::Value::get(
                        ctx->irCtx->builder.CreateLoad(argTy->get_llvm_type(), currArg->get_llvm()), argTy, false);
					if (not argTy->is_trivially_copyable()) {
						if (not isRefVar) {
							if (argValTy->is_reference()) {
								ctx->Error(
								    "This expression is a reference without variability and hence cannot be trivially moved from",
								    arguments[i - 1]->fileRange);
							} else {
								ctx->Error(
								    "This expression does not have variability and hence cannot be trivially moved from",
								    arguments[i - 1]->fileRange);
							}
						}
						ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(argTy->get_llvm_type()),
						                                argEmitLLVMValue);
					}
				} else {
					ctx->Error("This expression is not a value and is of type " + ctx->color(argTy->to_string()) +
					               " which is not trivially copyable or trivially movable. Please use " +
					               ctx->color("'copy") + " or " + ctx->color("'move") + " instead",
					           arguments[i - 1]->fileRange);
				}
			}
			SHOW("Argument value at " << i - 1 << " is of type " << argsEmit[i - 1]->get_ir_type()->to_string()
			                          << " and argtype is " << fnArgsTy.at(i)->get_type()->to_string())
			argVals.push_back(argsEmit[i - 1]->get_llvm());
		}
		if (isVariadicArg) {
			for (usize i = fnTy->get_argument_count(); i < argsEmit.size(); i++) {
				auto currArg  = argsEmit[i];
				auto argTy    = currArg->get_ir_type();
				auto isRefVar = false;
				if (argTy->is_reference()) {
					isRefVar = argTy->as_reference()->isSubtypeVariable();
					argTy    = argTy->as_reference()->get_subtype();
				} else {
					isRefVar = currArg->is_variable();
				}
				if (currArg->get_ir_type()->is_reference() || currArg->is_ghost_reference()) {
					if (currArg->get_ir_type()->is_reference()) {
						currArg->load_ghost_reference(ctx->irCtx->builder);
					}
					if (argTy->is_trivially_copyable() || argTy->is_trivially_movable()) {
						auto* argLLVMVal = currArg->get_llvm();
						argsEmit[i]      = ir::Value::get(
                            ctx->irCtx->builder.CreateLoad(argTy->get_llvm_type(), currArg->get_llvm()), argTy, false);
						if (not argTy->is_trivially_copyable()) {
							if (not isRefVar) {
								ctx->Error("This expression " +
								               String(currArg->get_ir_type()->is_reference()
								                          ? "is a reference without variability"
								                          : "does not have variability") +
								               " and hence cannot be trivially moved from",
								           arguments[i]->fileRange);
							}
							ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(argTy->get_llvm_type()),
							                                argLLVMVal);
						}
					} else {
						ctx->Error(
						    "This expression is of type " + ctx->color(currArg->get_ir_type()->to_string()) +
						        " which is not trivially copyable or movable. It cannot be passed as a variadic argument",
						    arguments[i]->fileRange);
					}
				}
				argVals.push_back(argsEmit[i]->get_llvm());
			}
		}
		return memFn->call(ctx->irCtx, argVals, localID, ctx->mod);
	} else if (inst->is_prerun_value() && instType->is_typed()) { // TODO: Support type traits for normal expressions
		return handle_type_wrap_functions(inst->as_prerun(), arguments, memberName, ctx, fileRange);
	} else if (instType->is_vector()) {
		auto*      origInst = ir::Value::get(inst->get_llvm(), inst->get_ir_type(), false);
		const auto isOrigRefNotVar =
		    (origInst->is_ghost_reference() && !origInst->is_variable()) ||
		    (origInst->is_reference() && !origInst->get_ir_type()->as_reference()->isSubtypeVariable());
		const auto shouldStore = origInst->is_ghost_reference() || origInst->is_reference();
		auto*      vecTy       = instType->as_vector();
		auto       refHandler  = [&]() {
            if (inst->is_reference() || inst->is_ghost_reference()) {
                inst->load_ghost_reference(ctx->irCtx->builder);
                if (inst->is_reference()) {
                    inst = ir::Value::get(ctx->irCtx->builder.CreateLoad(vecTy->get_llvm_type(), inst->get_llvm()),
					                             vecTy, false);
                }
            }
		};
		if (memberName.value == "insert") {
			if (isOrigRefNotVar) {
				ctx->Error(
				    "This " + String(origInst->is_ghost_reference() ? "expression" : "reference") +
				        " does not have variability and hence cannot be inserted into. Inserting vectors to values are possible, but for clarity, make sure to copy the vector first using " +
				        ctx->color("'copy"),
				    fileRange);
			}
			if (not vecTy->is_scalable()) {
				ctx->Error(
				    "Method " + ctx->color(memberName.value) +
				        " can only be called on expressions with scalable vector type. This expression is of type " +
				        ctx->color(vecTy->to_string()) + " which is not scalable",
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
			if (argVec->get_ir_type()->is_same(vecTy->get_non_scalable_type(ctx->irCtx)) ||
			    argVec->get_ir_type()->as_reference()->get_subtype()->is_same(
			        vecTy->get_non_scalable_type(ctx->irCtx))) {
				if (argVec->is_reference() || argVec->is_ghost_reference()) {
					argVec->load_ghost_reference(ctx->irCtx->builder);
					if (argVec->is_reference()) {
						argVec = ir::Value::get(
						    ctx->irCtx->builder.CreateLoad(vecTy->get_non_scalable_type(ctx->irCtx)->get_llvm_type(),
						                                   argVec->get_llvm()),
						    vecTy->get_non_scalable_type(ctx->irCtx), false);
					}
				}
				refHandler();
				auto result = ctx->irCtx->builder.CreateInsertVector(
				    vecTy->get_llvm_type(), inst->get_llvm(), argVec->get_llvm(),
				    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u));
				if (shouldStore) {
					if (origInst->is_reference()) {
						origInst->load_ghost_reference(ctx->irCtx->builder);
					}
					ctx->irCtx->builder.CreateStore(result, origInst->get_llvm());
					return origInst;
				} else {
					return ir::Value::get(result, vecTy, false);
				}
			} else {
				ctx->Error("The argument provided to " + ctx->color(memberName.value) + " is expected to be of type " +
				               ctx->color(vecTy->get_non_scalable_type(ctx->irCtx)->to_string()),
				           fileRange);
			}
		} else if (memberName.value == "reverse") {
			if (isOrigRefNotVar) {
				ctx->Error(
				    "This " + String(origInst->is_ghost_reference() ? "expression" : "reference") +
				        " does not have variability and hence cannot be reversed in-place. Reversing vector values are possible, but for clarity, make sure to copy the vector first using " +
				        ctx->color("'copy"),
				    fileRange);
			}
			if (!arguments.empty()) {
				ctx->Error("The method " + ctx->color(memberName.value) + " expects zero arguments, but " +
				               ctx->color(std::to_string(arguments.size())) + " values were provided",
				           fileRange);
			}
			refHandler();
			auto result = ctx->irCtx->builder.CreateVectorReverse(inst->get_llvm());
			if (shouldStore) {
				ctx->irCtx->builder.CreateStore(result, origInst->get_llvm());
				return origInst;
			} else {
				return ir::Value::get(result, vecTy, false);
			}
		} else {
			ctx->Error("Method " + ctx->color(memberName.value) + " is not supported for expression of type " +
			               ctx->color(vecTy->to_string()),
			           fileRange);
		}
	} else {
		ctx->Error("Method call for expression of type " + ctx->color(instType->to_string()) + " is not supported",
		           fileRange);
	}
	return nullptr;
}

Json MethodCall::to_json() const {
	Vec<JsonValue> args;
	for (auto* arg : arguments) {
		args.emplace_back(arg->to_json());
	}
	return Json()
	    ._("nodeType", "memberFunctionCall")
	    ._("instance", instance->to_json())
	    ._("function", memberName)
	    ._("arguments", args)
	    ._("hasCallNature", callNature.has_value())
	    ._("callNature", callNature.has_value() ? callNature.value() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
