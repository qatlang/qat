#include "./constructor.hpp"
#include "../show.hpp"
#include "./sentences/member_initialisation.hpp"
#include "node.hpp"
#include <vector>

namespace qat::ast {

ConstructorPrototype::~ConstructorPrototype() {
	for (auto* arg : arguments) {
		std::destroy_at(arg);
	}
}

void ConstructorPrototype::define(MethodState& state, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
	if (defineChecker) {
		auto defExp = defineChecker->emit(emitCtx);
		if (!defExp->get_ir_type()->is_bool()) {
			irCtx->Error("The define condition is required to be of type " + irCtx->color("bool") +
							 ", but instead got an expression of type " +
							 irCtx->color(defExp->get_ir_type()->to_string()),
						 defineChecker->fileRange);
		}
		state.defineCondition = llvm::cast<llvm::ConstantInt>(defExp->get_llvm_constant())->getValue().getBoolValue();
		if (!state.defineCondition.value()) {
			return;
		}
	}
	if (metaInfo.has_value()) {
		state.metaInfo = metaInfo.value().toIR(emitCtx);
	}
	if (type == ConstructorType::normal) {
		Vec<Pair<Maybe<bool>, ir::Type*>> generatedTypes;
		bool							  isMixAndHasMemberArg = false;
		SHOW("Generating types")
		for (usize i = 0; i < arguments.size(); i++) {
			auto arg = arguments[i];
			if (arg->is_member_arg()) {
				if (isMixAndHasMemberArg) {
					irCtx->Error("Cannot have multiple member arguments in a constructor for type " +
									 irCtx->color(state.parent->get_parent_type()->to_string()) +
									 ", since it is a mix type",
								 arg->get_name().range);
				}
				SHOW("Arg is type member")
				if (!state.parent->get_parent_type()->is_struct() && !state.parent->get_parent_type()->is_mix()) {
					irCtx->Error(
						"The parent type is not a struct or mix type, and hence member argument syntax cannot be used here",
						arg->get_name().range);
				}
				if (state.parent->get_parent_type()->is_struct()) {
					auto* coreType = state.parent->get_parent_type()->as_struct();
					if (coreType->has_field_with_name(arg->get_name().value)) {
						for (auto mem : presentMembers) {
							if (mem->name.value == arg->get_name().value) {
								irCtx->Error("The member " + irCtx->color(mem->name.value) + " is repeating here",
											 arg->get_name().range);
							}
						}
						auto* mem = coreType->get_field_with_name(arg->get_name().value);
						mem->add_mention(arg->get_name().range);
						presentMembers.push_back(coreType->get_field_with_name(arg->get_name().value));
						SHOW("Getting core type member: " << arg->get_name().value)
						generatedTypes.push_back({None, mem->type});
					} else {
						irCtx->Error("No member field named " + arg->get_name().value + " in the core type " +
										 coreType->get_full_name(),
									 arg->get_name().range);
					}
				} else {
					auto mixTy = state.parent->get_parent_type()->as_mix();
					auto check = mixTy->has_variant_with_name(arg->get_name().value);
					if (!check.first) {
						irCtx->Error("No variant named " + irCtx->color(arg->get_name().value) +
										 " is present in mix type " + irCtx->color(mixTy->to_string()),
									 arg->get_name().range);
					}
					if (!check.second) {
						irCtx->Error(
							"The variant " + irCtx->color(arg->get_name().value) + " of mix type " +
								irCtx->color(mixTy->to_string()) +
								" does not have a type associated with it and hence cannot be used as a member argument",
							fileRange);
					}
					isMixAndHasMemberArg = true;
				}
			} else if (arg->is_variadic_arg()) {
				if (i != (arguments.size() - 1)) {
					irCtx->Error("Variadic argument should always be the last argument", arg->get_name().range);
				}
				if (state.parent->is_done_skill() &&
					state.parent->as_done_skill()->has_from_convertor(None, generatedTypes.back().second)) {
					irCtx->Error(
						"Because of the variadic argument, this constructor can be called with the same argument as a previous " +
							irCtx->color("from convertor") + ", thereby effectively having the same signature",
						fileRange,
						Pair<String, FileRange>{"The previous " + irCtx->color("from convertor") + " can be found here",
												state.parent->as_done_skill()
													->get_from_convertor(None, generatedTypes.back().second)
													->get_name()
													.range});
				} else if (state.parent->is_expanded() &&
						   state.parent->as_expanded()->has_from_convertor(None, generatedTypes.back().second)) {
					irCtx->Error(
						"Because of the variadic argument, this constructor can be called with the same argument as a previous " +
							irCtx->color("from convertor") + ", thereby effectively having the same signature",
						fileRange,
						Pair<String, FileRange>{"The previous " + irCtx->color("from convertor") + " can be found here",
												state.parent->as_expanded()
													->get_from_convertor(None, generatedTypes.back().second)
													->get_name()
													.range});
				}
			} else {
				generatedTypes.push_back({None, arg->get_type()->emit(emitCtx)});
			}
			SHOW("Generated type of the argument is " << generatedTypes.back().second->to_string())
		}
		SHOW("Argument types generated")
		if (state.parent->is_done_skill() &&
			state.parent->as_done_skill()->has_constructor_with_types(generatedTypes)) {
			irCtx->Error(
				"A constructor with the same signature exists in the same implementation " +
					irCtx->color(state.parent->as_done_skill()->to_string()),
				fileRange,
				Pair<String, FileRange>{
					"The existing constructor can be found here",
					state.parent->as_done_skill()->get_constructor_with_types(generatedTypes)->get_name().range});
		}
		if (state.parent->get_parent_type()->is_expanded() &&
			state.parent->get_parent_type()->as_expanded()->has_constructor_with_types(generatedTypes)) {
			irCtx->Error("A constructor with the same signature exists in the parent type " +
							 irCtx->color(state.parent->get_parent_type()->to_string()),
						 fileRange,
						 Pair<String, FileRange>{"The existing constructor can be found here",
												 state.parent->get_parent_type()
													 ->as_expanded()
													 ->get_constructor_with_types(generatedTypes)
													 ->get_name()
													 .range});
		}
		if (state.parent->get_parent_type()->is_struct()) {
			auto  coreType	 = state.parent->get_parent_type()->as_struct();
			auto& allMembers = coreType->get_members();
			for (auto* mem : allMembers) {
				bool foundMem = false;
				for (auto presentMem : presentMembers) {
					if (mem == presentMem) {
						foundMem = true;
						break;
					}
				}
				if (!foundMem) {
					if (mem->type->has_prerun_default_value() || mem->type->is_default_constructible()) {
						absentMembersWithDefault.push_back(mem);
					} else {
						absentMembersWithoutDefault.push_back(mem);
					}
				}
			}
		}
		Vec<ir::Argument> args;
		SHOW("Setting variability of arguments")
		for (usize i = 0; i < arguments.size(); i++) {
			auto arg = arguments[i];
			if (arg->is_member_arg()) {
				SHOW("Creating member argument")
				args.push_back(ir::Argument::CreateMember(arg->get_name(), generatedTypes.at(i).second, i));
			} else if (arguments[i]->is_variadic_arg()) {
				if (i != (arguments.size() - 1)) {
					irCtx->Error("Variadic arguments should always be the last argument",
								 arguments[i]->get_name().range);
				}
				args.push_back(ir::Argument::CreateVariadic(arg->get_name().value, arg->get_name().range, i));
			} else {
				args.push_back(arguments.at(i)->is_variable()
								   ? ir::Argument::CreateVariable(arg->get_name(), generatedTypes[i].second, i)
								   : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes[i].second, i));
			}
		}
		SHOW("About to create function")
		state.result = ir::Method::CreateConstructor(state.parent, nameRange,
													 state.metaInfo.has_value() && state.metaInfo->get_inline(), args,
													 fileRange, emitCtx->get_visibility_info(visibSpec), irCtx);
		SHOW("Constructor created in the IR")
	} else if (type == ConstructorType::Default) {
		if (state.parent->is_done_skill() && state.parent->get_parent_type()->is_expanded() &&
			state.parent->get_parent_type()->as_expanded()->has_default_constructor()) {
			irCtx->Error("The target type of this implementation already has a " + irCtx->color("default") +
							 " constructor, so it cannot be defined again",
						 fileRange);
		}
		state.result = ir::Method::DefaultConstructor(state.parent, nameRange,
													  state.metaInfo.has_value() && state.metaInfo->get_inline(),
													  emitCtx->get_visibility_info(visibSpec), fileRange, irCtx);
	} else if (type == ConstructorType::copy) {
		state.result = ir::Method::CopyConstructor(state.parent, nameRange,
												   state.metaInfo.has_value() && state.metaInfo->get_inline(),
												   argName.value(), fileRange, irCtx);
	} else if (type == ConstructorType::move) {
		state.result = ir::Method::MoveConstructor(state.parent, nameRange,
												   state.metaInfo.has_value() && state.metaInfo->get_inline(),
												   argName.value(), fileRange, irCtx);
	}
}

Json ConstructorPrototype::to_json() const {
	Vec<JsonValue> args;
	for (auto* arg : arguments) {
		args.push_back(arg->to_json());
	}
	return Json()
		._("nodeType", "constructorPrototype")
		._("arguments", args)
		._("hasVisibility", visibSpec.has_value())
		._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
		._("fileRange", fileRange);
}

void ConstructorDefinition::define(MethodState& state, ir::Ctx* irCtx) { prototype->define(state, irCtx); }

ir::Value* ConstructorDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
	if (state.defineCondition.has_value() && !state.defineCondition.value()) {
		return nullptr;
	}
	auto* fnEmit = state.result;
	SHOW("FNemit is " << fnEmit)
	SHOW("Set active contructor: " << fnEmit->get_full_name())
	auto* block = new ir::Block(fnEmit, nullptr);
	block->set_file_range(fileRange);
	SHOW("Created entry block")
	block->set_active(irCtx->builder);
	SHOW("Set new block as the active block")
	SHOW("About to allocate necessary arguments")
	auto  argIRTypes  = fnEmit->get_ir_type()->as_function()->get_argument_types();
	auto* parentRefTy = argIRTypes.at(0)->get_type()->as_reference();
	auto* self = block->new_value("''", parentRefTy, false, parentRefTy->get_subtype()->as_struct()->get_name().range);
	SHOW("Storing self instance")
	irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(0u), self->get_llvm());
	SHOW("Loading implicit ptr")
	self->load_ghost_reference(irCtx->builder);
	SHOW("Loaded self instance")
	if ((prototype->type == ConstructorType::copy) || (prototype->type == ConstructorType::move)) {
		auto* argVal = block->new_value(
			prototype->argName->value,
			ir::ReferenceType::get(prototype->type == ConstructorType::move, state.parent->get_parent_type(), irCtx),
			false, prototype->argName->range);
		irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(1), argVal->get_alloca(), false);
		argVal->load_ghost_reference(irCtx->builder);
	} else {
		for (usize i = 1; i < argIRTypes.size(); i++) {
			SHOW("Argument type is " << argIRTypes[i]->get_type()->to_string())
			if (argIRTypes[i]->is_member_argument()) {
				llvm::Value* memPtr = nullptr;
				if (parentRefTy->get_subtype()->is_struct()) {
					memPtr = irCtx->builder.CreateStructGEP(
						parentRefTy->get_subtype()->get_llvm_type(), self->get_llvm(),
						parentRefTy->get_subtype()->as_struct()->get_index_of(argIRTypes[i]->get_name()).value());
					fnEmit->add_init_member(
						{parentRefTy->get_subtype()->as_struct()->get_field_index(argIRTypes[i]->get_name()),
						 prototype->arguments[i - 1]->get_name().range});
				} else if (parentRefTy->get_subtype()->is_mix()) {
					memPtr = irCtx->builder.CreatePointerCast(
						irCtx->builder.CreateStructGEP(parentRefTy->get_subtype()->get_llvm_type(), self->get_llvm(),
													   1u),
						parentRefTy->get_subtype()
							->as_mix()
							->get_variant_with_name(argIRTypes[i]->get_name())
							->get_llvm_type()
							->getPointerTo(irCtx->dataLayout.value().getProgramAddressSpace()));
					fnEmit->add_init_member(
						{parentRefTy->get_subtype()->as_mix()->get_index_of(argIRTypes[i]->get_name()),
						 prototype->arguments[i]->get_name().range});
				} else {
					irCtx->Error("Cannot use member argument syntax for the parent type " +
									 irCtx->color(parentRefTy->get_subtype()->to_string()) +
									 " as it is not a mix or core type",
								 prototype->arguments[i - 1]->get_name().range);
				}
				irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), memPtr);
			} else if (not argIRTypes[i]->is_variadic_argument()) {
				SHOW("Argument is variable")
				auto* argVal = block->new_value(argIRTypes[i]->get_name(), argIRTypes[i]->get_type(),
												prototype->arguments[i - 1]->is_variable(),
												prototype->arguments[i - 1]->get_name().range);
				SHOW("Created local value for the argument")
				irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->get_alloca(), false);
			}
		}
	}
	for (auto sent : sentences) {
		if (sent->nodeType() == NodeType::MEMBER_INIT) {
			((ast::MemberInit*)sent)->isAllowed = true;
		}
	}
	emit_sentences(
		sentences,
		EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(fnEmit));
	if (state.parent->get_parent_type()->is_struct()) {
		SHOW("Setting default values for fields")
		auto coreTy = state.parent->get_parent_type()->as_struct();
		for (usize i = 0; i < coreTy->get_field_count(); i++) {
			auto mem = coreTy->get_field_at(i);
			if (fnEmit->is_member_initted(i)) {
				continue;
			}
			if (mem->defaultValue.has_value()) {
				if (mem->defaultValue.value()->has_type_inferrance()) {
					mem->defaultValue.value()->as_type_inferrable()->set_inference_type(mem->type);
				}
				auto* memVal = mem->defaultValue.value()->emit(
					EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent));
				if (memVal->get_ir_type()->is_same(mem->type)) {
					if (memVal->is_ghost_reference()) {
						if (mem->type->is_trivially_copyable() || mem->type->is_trivially_movable()) {
							irCtx->builder.CreateStore(
								irCtx->builder.CreateLoad(mem->type->get_llvm_type(), memVal->get_llvm()),
								irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
							if (!mem->type->is_trivially_copyable()) {
								if (!memVal->is_variable()) {
									irCtx->Error(
										"This expression does not have variability and hence cannot be trivially moved from",
										mem->defaultValue.value()->fileRange);
								}
								irCtx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->get_llvm_type()),
														   memVal->get_llvm());
							}
						} else {
							irCtx->Error("This expression is of type " +
											 irCtx->color(memVal->get_ir_type()->to_string()) +
											 " which is not trivially copyable or trivially movable. Please use " +
											 irCtx->color("'copy") + " or " + irCtx->color("'move") + " accordingly",
										 mem->defaultValue.value()->fileRange);
						}
					} else {
						irCtx->builder.CreateStore(
							memVal->get_llvm(),
							irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
					}
				} else if (memVal->is_reference() &&
						   memVal->get_ir_type()->as_reference()->get_subtype()->is_same(mem->type)) {
					if (mem->type->is_trivially_copyable() || mem->type->is_trivially_movable()) {
						irCtx->builder.CreateStore(
							irCtx->builder.CreateLoad(mem->type->get_llvm_type(), memVal->get_llvm()),
							irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
						if (!mem->type->is_trivially_copyable()) {
							if (!memVal->get_ir_type()->as_reference()->isSubtypeVariable()) {
								irCtx->Error(
									"This expression is a reference without variability and hence cannot be trivially moved from",
									mem->defaultValue.value()->fileRange);
							}
							irCtx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->get_llvm_type()),
													   memVal->get_llvm());
						}
					} else {
						irCtx->Error("This expression is a reference to type " + irCtx->color(mem->type->to_string()) +
										 " which is not trivially copyable or trivially movable. Please use " +
										 irCtx->color("'copy") + " or " + irCtx->color("'move") + " accordingly",
									 mem->defaultValue.value()->fileRange);
					}
				} else if (mem->type->is_reference() &&
						   mem->type->as_reference()->get_subtype()->is_same(memVal->get_ir_type()) &&
						   memVal->is_ghost_reference() &&
						   (mem->type->as_reference()->isSubtypeVariable() ? memVal->is_variable() : true)) {
					irCtx->builder.CreateStore(memVal->get_llvm(), irCtx->builder.CreateStructGEP(
																	   coreTy->get_llvm_type(), self->get_llvm(), i));
				} else {
					irCtx->Error("The expected type of the member field is " + irCtx->color(mem->type->to_string()) +
									 " but the value provided is of type " +
									 irCtx->color(memVal->get_ir_type()->to_string()),
								 FileRange{mem->name.range, mem->defaultValue.value()->fileRange});
				}
				fnEmit->add_init_member({i, mem->defaultValue.value()->fileRange});
			} else if (mem->type->has_prerun_default_value() || mem->type->is_default_constructible()) {
				if (mem->type->has_prerun_default_value()) {
					irCtx->Warning("Member field " + irCtx->highlightWarning(mem->name.value) +
									   " is initialised using the default value of type " +
									   irCtx->highlightWarning(mem->type->to_string()) +
									   " at the end of this constructor. Try using " +
									   irCtx->highlightWarning("default") +
									   " instead to explicitly initialise the member field",
								   fileRange);
					irCtx->builder.CreateStore(
						mem->type->get_prerun_default_value(irCtx)->get_llvm(),
						irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i));
				} else {
					irCtx->Warning("Member field " + irCtx->highlightWarning(mem->name.value) +
									   " is default constructed at the end of this constructor. Try using " +
									   irCtx->highlightWarning("default") +
									   " instead to explicitly initialise the member field",
								   fileRange);
					mem->type->default_construct_value(
						irCtx,
						ir::Value::get(irCtx->builder.CreateStructGEP(coreTy->get_llvm_type(), self->get_llvm(), i),
									   ir::ReferenceType::get(true, mem->type, irCtx), false),
						fnEmit);
				}
				fnEmit->add_init_member({i, fileRange});
			}
		}
	}
	if (state.parent->get_parent_type()->is_struct()) {
		Vec<Pair<String, FileRange>> missingMembers;
		auto						 cTy = state.parent->get_parent_type()->as_struct();
		for (auto ind = 0; ind < cTy->get_field_count(); ind++) {
			auto memCheck = fnEmit->is_member_initted(ind);
			if (!memCheck.has_value()) {
				missingMembers.push_back({cTy->get_field_at(ind)->name.value, fileRange});
			}
		}
		if (!missingMembers.empty()) {
			Vec<ir::QatError> errors;
			for (usize i = 0; i < missingMembers.size(); i++) {
				errors.push_back(ir::QatError("Member field " + irCtx->color(missingMembers[i].first) +
												  " of parent type " + irCtx->color(cTy->get_full_name()) +
												  " is not initialised in this constructor",
											  missingMembers[i].second));
			}
			irCtx->Errors(errors);
		}
	} else if (state.parent->get_parent_type()->is_mix()) {
		bool isMixInitialised = false;
		for (usize i = 0; i < state.parent->get_parent_type()->as_mix()->get_variant_count(); i++) {
			auto memRes = fnEmit->is_member_initted(i);
			if (memRes.has_value()) {
				isMixInitialised = true;
				break;
			}
		}
		if (!isMixInitialised) {
			irCtx->Error("Mix type is not initialised in this constructor", fileRange);
		}
	}
	ir::function_return_handler(irCtx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
	SHOW("Sentences emitted")
	return nullptr;
}

Json ConstructorDefinition::to_json() const {
	Vec<JsonValue> sntcs;
	for (auto* sentence : sentences) {
		sntcs.push_back(sentence->to_json());
	}
	return Json()
		._("nodeType", "constructorDefinition")
		._("prototype", prototype->to_json())
		._("body", sntcs)
		._("fileRange", fileRange);
}

} // namespace qat::ast
