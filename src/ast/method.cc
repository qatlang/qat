#include "./method.hpp"
#include "../IR/types/struct_type.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "./emit_ctx.hpp"
#include "./types/self_type.hpp"
#include "./types/subtype.hpp"

#include <vector>

namespace qat::ast {

MethodPrototype::MethodPrototype(MethodType _fnTy, Identifier _name, PrerunExpression* _condition,
                                 Vec<Argument*> _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : fnTy(_fnTy), name(std::move(_name)), arguments(std::move(_arguments)), isVariadic(_isVariadic),
      returnType(_returnType), visibSpec(_visibSpec), fileRange(_fileRange), defineChecker(_condition),
      metaInfo(_metaInfo) {}

MethodPrototype::~MethodPrototype() {
	for (auto* arg : arguments) {
		std::destroy_at(arg);
	}
}

MethodPrototype* MethodPrototype::Normal(bool _isVariationFn, const Identifier& _name, PrerunExpression* _condition,
                                         const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                         Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
	return std::construct_at(OwnNormal(MethodPrototype), _isVariationFn ? MethodType::variation : MethodType::normal,
	                         _name, _condition, _arguments, _isVariadic, _returnType, _metaInfo, visibSpec, _fileRange);
}

MethodPrototype* MethodPrototype::Static(const Identifier& _name, PrerunExpression* _condition,
                                         const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                         Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
	return std::construct_at(OwnNormal(MethodPrototype), MethodType::Static, _name, _condition, _arguments, _isVariadic,
	                         _returnType, _metaInfo, visibSpec, _fileRange);
}

MethodPrototype* MethodPrototype::Value(const Identifier& _name, PrerunExpression* _condition,
                                        const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                        Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> visibSpec,
                                        const FileRange& _fileRange) {
	return std::construct_at(OwnNormal(MethodPrototype), MethodType::valued, _name, _condition, _arguments, _isVariadic,
	                         _returnType, _metaInfo, visibSpec, _fileRange);
}

void MethodPrototype::define(MethodState& state, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
	if (defineChecker) {
		auto condRes = defineChecker->emit(emitCtx);
		if (not condRes->get_ir_type()->is_bool()) {
			irCtx->Error("The condition for defining the method should be of " + irCtx->color("bool") + " type",
			             defineChecker->fileRange);
		}
		state.defineCondition = llvm::cast<llvm::ConstantInt>(condRes->get_llvm())->getValue().getBoolValue();
		if (state.defineCondition.has_value() && not state.defineCondition.value()) {
			return;
		}
	}
	if (metaInfo.has_value()) {
		state.metaInfo = metaInfo.value().toIR(emitCtx);
	}
	SHOW("Defining member proto " << name.value << " " << state.parent->get_parent_type()->to_string())
	ir::SkillMethod* skillMethod = nullptr;
	if (state.parent->is_done_skill()) {
		auto doneSkill = state.parent->as_done_skill();
		if (doneSkill->is_normal_skill() && (fnTy != MethodType::valued)) {
			if (doneSkill->get_skill()->has_prototype(name.value, get_skill_method_kind_for(fnTy))) {
				skillMethod = doneSkill->get_skill()->get_prototype(name.value, get_skill_method_kind_for(fnTy));
			} else if (doneSkill->get_skill()->has_any_prototype(name.value)) {
				irCtx->Error("Could not find a " + irCtx->color(method_type_to_string(fnTy)) + " method in the skill " +
				                 irCtx->color(doneSkill->get_skill()->get_full_name()) + " with the name " +
				                 irCtx->color(name.value) +
				                 ". Found another method in the skill with the same name. Please check your logic",
				             name.range);
			}
		}
		if ((fnTy == MethodType::variation) && doneSkill->has_variation_method(name.value)) {
			irCtx->Error(
			    "A variation method named " + irCtx->color(name.value) + " already exists in the same implementation",
			    name.range,
			    std::make_optional(std::make_pair("The implementation can be found here",
			                                      doneSkill->get_variation_method(name.value)->get_name().range)));
		}
		if ((fnTy == MethodType::normal) && doneSkill->has_normal_method(name.value)) {
			irCtx->Error(
			    "A method named " + irCtx->color(name.value) + " already exists in the same implementation", name.range,
			    std::make_optional(std::make_pair("The implementation can be found here",
			                                      doneSkill->get_variation_method(name.value)->get_name().range)));
		}
		if (doneSkill->has_static_method(name.value)) {
			irCtx->Error(
			    "A static function named " + irCtx->color(name.value) +
			        " already exists in the same implementation. Static functions and other methods cannot share names",
			    name.range,
			    std::make_optional(std::make_pair("The implementation can be found here",
			                                      doneSkill->get_variation_method(name.value)->get_name().range)));
		}
	}
	SHOW("Getting parent type")
	auto* parentType =
	    state.parent->is_done_skill() ? state.parent->as_done_skill()->get_ir_type() : state.parent->as_expanded();
	if (parentType->is_expanded()) {
		auto expTy = parentType->as_expanded();
		if ((fnTy == MethodType::variation) && expTy->has_variation(name.value)) {
			irCtx->Error("A variation function named " + irCtx->color(name.value) + " exists in the parent type " +
			                 irCtx->color(expTy->get_full_name()) + " at " +
			                 irCtx->color(expTy->get_variation(name.value)->get_name().range.start_to_string()),
			             name.range);
		}
		if ((fnTy == MethodType::normal) && expTy->has_normal_method(name.value)) {
			irCtx->Error("A member function named " + irCtx->color(name.value) + " exists in the parent type " +
			                 irCtx->color(expTy->get_full_name()) + " at " +
			                 irCtx->color(expTy->get_normal_method(name.value)->get_name().range.start_to_string()),
			             name.range);
		}
		if (expTy->has_static_method(name.value)) {
			irCtx->Error("A static function named " + irCtx->color(name.value) + " exists in the parent type " +
			                 irCtx->color(expTy->get_full_name()) + " at " +
			                 irCtx->color(expTy->get_static_method(name.value)->get_name().range.start_to_string()),
			             name.range);
		}
		if (expTy->is_struct()) {
			if (expTy->as_struct()->has_field_with_name(name.value) ||
			    expTy->as_struct()->has_static_field(name.value)) {
				irCtx->Error(
				    String(expTy->as_struct()->has_field_with_name(name.value) ? "Member" : "Static") +
				        " field named " + irCtx->color(name.value) + " exists in the parent type " +
				        irCtx->color(expTy->to_string()) + ". Try if you can change the name of this function" +
				        (state.parent->is_done_skill() && state.parent->as_done_skill()->is_normal_skill()
				             ? (" in the skill " +
				                irCtx->color(state.parent->as_done_skill()->get_skill()->get_full_name()) + " at " +
				                irCtx->color(
				                    state.parent->as_done_skill()->get_skill()->get_name().range.start_to_string()))
				             : ""),
				    name.range);
			}
		}
	}
	SHOW("Checking self type")
	bool isSelfReturn = false;
	if (returnType.has_value() && returnType.value()->type_kind() == AstTypeKind::SELF_TYPE) {
		SHOW("Return is self")
		auto* selfRet = (SelfType*)returnType.value();
		if (not selfRet->isJustType) {
			selfRet->isVarRef          = fnTy == MethodType::variation;
			selfRet->canBeSelfInstance = (fnTy == MethodType::normal) || (fnTy == MethodType::variation);
			isSelfReturn               = true;
		}
	}
	SHOW("Emitting return type")
	auto* retTy = returnType.has_value() ? returnType.value()->emit(emitCtx) : ir::VoidType::get(irCtx->llctx);
	if (skillMethod) {
		if (not skillMethod->get_given_type().irType->is_same(retTy)) {
			irCtx->Error(
			    "Expected the type " + irCtx->color(skillMethod->get_given_type().irType->to_string()) +
			        " to be the given type of this method according to the corresponding method in the skill " +
			        irCtx->color(skillMethod->get_parent_skill()->get_full_name()) + ", but got " +
			        irCtx->color(retTy->to_string()) + " instead as the given type here. The expected signature is " +
			        irCtx->color(skillMethod->to_string()),
			    returnType.has_value() ? returnType.value()->fileRange : fileRange,
			    skillMethod->get_given_type().astType
			        ? std::make_optional(
			              std::make_pair("The given type of the corresponding method in the skill can be found here",
			                             skillMethod->get_given_type().astType->fileRange))
			        : None);
		}
		if (skillMethod->get_arg_count() != arguments.size()) {
			irCtx->Error(
			    "The signature of this method does not match the signature of the corresponding method found in the skill " +
			        irCtx->color(skillMethod->get_parent_skill()->get_full_name()) + ". The expected signature is " +
			        irCtx->color(skillMethod->to_string()),
			    fileRange);
		}
	}
	SHOW("Generating types")
	Vec<ir::Type*> generatedTypes;
	for (usize i = 0; i < arguments.size(); i++) {
		auto arg = arguments[i];
		if (arg->is_member_arg()) {
			if (not state.parent->get_parent_type()->is_struct()) {
				irCtx->Error(
				    "The parent type of this function is not a struct type and hence the member argument syntax cannot be used",
				    arg->get_name().range);
			}
			if (fnTy != MethodType::Static && fnTy != MethodType::valued) {
				auto structType = state.parent->get_parent_type()->as_struct();
				if (structType->has_field_with_name(arg->get_name().value)) {
					if (state.parent->is_done_skill()) {
						if (not structType->get_field_with_name(arg->get_name().value)
						            ->visibility.is_accessible(emitCtx->get_access_info())) {
							irCtx->Error("The member field " + irCtx->color(arg->get_name().value) +
							                 " of parent type " + irCtx->color(structType->to_string()) +
							                 " is not accessible here",
							             arg->get_name().range);
						}
					}
					if (fnTy == MethodType::variation) {
						auto* memTy = structType->get_type_of_field(arg->get_name().value);
						if (memTy->is_ref()) {
							if (memTy->as_ref()->has_variability()) {
								memTy = memTy->as_ref()->get_subtype();
							} else {
								irCtx->Error("Member " + irCtx->color(arg->get_name().value) + " of struct type " +
								                 irCtx->color(structType->get_full_name()) +
								                 " is not a variable reference and hence cannot "
								                 "be reassigned",
								             arg->get_name().range);
							}
						}
						generatedTypes.push_back(memTy);
					} else {
						irCtx->Error("This method is not marked as a variation. It "
						             "cannot use the member argument syntax",
						             fileRange);
					}
				} else {
					irCtx->Error("No non-static member named " + arg->get_name().value + " in the struct type " +
					                 structType->get_full_name(),
					             arg->get_name().range);
				}
			} else {
				irCtx->Error("Function " + name.value + " is not a normal or variation method of type " +
				                 state.parent->get_parent_type()->to_string() +
				                 ". So it cannot use the member argument syntax",
				             arg->get_name().range);
			}
		} else if (arg->is_variadic_arg()) {
			if (i != (arguments.size() - 1)) {
				irCtx->Error("Variadic argument should always be the last argument", arg->get_name().range);
			}
			if (skillMethod && (skillMethod->get_args().back()->kind != ir::SkillArgKind::VARIADIC)) {
				irCtx->Error("The method found in the skill " +
				                 irCtx->color(skillMethod->get_parent_skill()->get_full_name()) + " with the name " +
				                 irCtx->color(name.value) +
				                 " does not have a variadic argument, but a variadic argument has been provided here",
				             arg->get_name().range);
			}
		} else {
			auto argIRTy = arg->get_type()->emit(emitCtx);
			if (skillMethod) {
				auto skArg = skillMethod->get_arg_at(i);
				if (skArg->isVar != arg->is_variable()) {
					irCtx->Error("In the method named " + irCtx->color(name.value) + " in the skill " +
					                 irCtx->color(skillMethod->get_parent_skill()->get_full_name()) +
					                 ", the corresponding argument " + skArg->name.value +
					                 (skArg->isVar ? " has variability" : " does not have variability") +
					                 ", but the argument provided here " +
					                 (arg->is_variable() ? " has variability" : " does not have variability"),
					             arg->get_name().range);
				}
				if (not skArg->type.irType->is_same(argIRTy)) {
					irCtx->Error(
					    "Expected the argument to be of type " + irCtx->color(skArg->type.irType->to_string()) +
					        ", but instead got an argument of type " + irCtx->color(argIRTy->to_string()) + " here",
					    arg->get_name().range);
				}
			}
			generatedTypes.push_back(argIRTy);
		}
	}
	SHOW("Argument types generated")
	Vec<ir::Argument> args;
	SHOW("Setting variability of arguments")
	for (usize i = 0; i < arguments.size(); i++) {
		if (arguments[i]->is_member_arg()) {
			SHOW("Argument at " << i << " named " << arguments[i]->get_name().value << " is a type member")
			args.push_back(ir::Argument::CreateMember(arguments[i]->get_name(), generatedTypes[i], i));
		} else if (arguments[i]->is_variadic_arg()) {
			args.push_back(
			    ir::Argument::CreateVariadic(arguments[i]->get_name().value, arguments[i]->get_name().range, i));
		} else {
			SHOW("Argument at " << i << " named " << arguments.at(i)->get_name().value << " is not a type member")
			args.push_back(arguments.at(i)->is_variable()
			                   ? ir::Argument::CreateVariable(arguments.at(i)->get_name(),
			                                                  arguments.at(i)->get_type()->emit(emitCtx), i)
			                   : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes.at(i), i));
		}
	}
	SHOW("Variability setting complete")
	SHOW("About to create function")
	if (fnTy == MethodType::Static) {
		SHOW("MemberFn :: " << name.value << " Static Method")
		state.result =
		    ir::Method::CreateStatic(state.parent, name, state.metaInfo.has_value() && state.metaInfo->get_inline(),
		                             retTy, args, fileRange, emitCtx->get_visibility_info(visibSpec), irCtx);
	} else if (fnTy == MethodType::valued) {
		SHOW("MemberFn :: " << name.value << " Valued Method")
		if (not parentType->has_simple_copy()) {
			irCtx->Error("The parent type does not have simple-copy and hence cannot have value methods", fileRange);
		}
		state.result =
		    ir::Method::CreateValued(state.parent, name, state.metaInfo.has_value() && state.metaInfo->get_inline(),
		                             retTy, args, fileRange, emitCtx->get_visibility_info(visibSpec), irCtx);
	} else {
		SHOW("MemberFn :: " << name.value << " Method or Variation")
		state.result = ir::Method::Create(state.parent, fnTy == MethodType::variation, name,
		                                  state.metaInfo.has_value() && state.metaInfo->get_inline(),
		                                  ir::ReturnType::get(retTy, isSelfReturn), args, fileRange,
		                                  emitCtx->get_visibility_info(visibSpec), irCtx);
	}
	state.result->skillMethod = skillMethod;
}

Json MethodPrototype::to_json() const {
	Vec<JsonValue> args;
	for (auto* arg : arguments) {
		args.push_back(arg->to_json());
	}
	return Json()
	    ._("nodeType", "methodPrototype")
	    ._("functionType", method_type_to_string(fnTy))
	    ._("name", name)
	    ._("hasReturnType", returnType.has_value())
	    ._("returnType", returnType.has_value() ? returnType.value()->to_json() : JsonValue())
	    ._("arguments", args)
	    ._("isVariadic", isVariadic);
}

void MethodDefinition::define(MethodState& state, ir::Ctx* irCtx) { prototype->define(state, irCtx); }

ir::Value* MethodDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
	if (state.defineCondition.has_value() && not state.defineCondition.value()) {
		return nullptr;
	}
	auto* fnEmit = state.result;
	SHOW("Set active member function: " << fnEmit->get_full_name())
	auto* block = ir::Block::create(fnEmit, nullptr);
	SHOW("Created entry block")
	block->set_active(irCtx->builder);
	SHOW("Set new block as the active block")
	SHOW("About to allocate necessary arguments")
	auto            argIRTypes  = fnEmit->get_ir_type()->as_function()->get_argument_types();
	ir::RefType*    structRefTy = nullptr;
	ir::LocalValue* self        = nullptr;
	if (prototype->fnTy != MethodType::Static && prototype->fnTy != MethodType::valued) {
		structRefTy = argIRTypes.at(0)->get_type()->as_ref();
		self = block->new_local("''", structRefTy, false, structRefTy->get_subtype()->as_struct()->get_name().range);
		irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(0u), self->get_llvm());
		self->load_ghost_ref(irCtx->builder);
	}
	SHOW("Arguments size is " << argIRTypes.size())
	for (usize i = 1; i < argIRTypes.size(); i++) {
		SHOW("Argument name in member function is " << argIRTypes.at(i)->get_name())
		SHOW("Argument type in member function is " << argIRTypes.at(i)->get_type()->to_string())
		if (argIRTypes.at(i)->is_member_argument()) {
			auto* memPtr = irCtx->builder.CreateStructGEP(
			    structRefTy->get_subtype()->get_llvm_type(), self->get_llvm(),
			    structRefTy->get_subtype()->as_struct()->get_index_of(argIRTypes.at(i)->get_name()).value());
			auto* memTy = structRefTy->get_subtype()->as_struct()->get_type_of_field(argIRTypes.at(i)->get_name());
			if (memTy->is_ref()) {
				memPtr = irCtx->builder.CreateLoad(memTy->as_ref()->get_llvm_type(), memPtr);
			}
			irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), memPtr, false);
		} else if (not argIRTypes.at(i)->is_variadic_argument()) {
			if (not argIRTypes.at(i)->get_type()->has_simple_copy() || argIRTypes.at(i)->is_variable()) {
				auto* argVal =
				    block->new_local(argIRTypes.at(i)->get_name(), argIRTypes.at(i)->get_type(),
				                     argIRTypes.at(i)->is_variable(), prototype->arguments.at(i - 1)->get_name().range);
				SHOW("Created local value for the argument")
				irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->get_alloca(), false);
			}
		}
	}
	emit_sentences(
	    sentences,
	    EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(fnEmit));
	ir::function_return_handler(irCtx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
	SHOW("Sentences emitted")
	return nullptr;
}

Json MethodDefinition::to_json() const {
	Vec<JsonValue> sntcs;
	for (auto* sentence : sentences) {
		sntcs.push_back(sentence->to_json());
	}
	return Json()
	    ._("nodeType", "memberDefinition")
	    ._("prototype", prototype->to_json())
	    ._("body", sntcs)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
