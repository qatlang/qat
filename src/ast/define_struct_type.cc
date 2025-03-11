#include "./define_struct_type.hpp"
#include "../IR/generics.hpp"
#include "../IR/types/struct_type.hpp"
#include "../utils/identifier.hpp"
#include "./constructor.hpp"
#include "./convertor.hpp"
#include "./destructor.hpp"
#include "./member_parent_like.hpp"
#include "./method.hpp"
#include "./node.hpp"
#include "./operator_function.hpp"
#include "./type_definition.hpp"
#include "./types/generic_abstract.hpp"

namespace qat::ast {

Json DefineStructType::Member::to_json() const {
	return Json()
	    ._("nodeType", "structTypeMember")
	    ._("name", name)
	    ._("type", type->to_json())
	    ._("variability", variability)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

Json DefineStructType::StaticMember::to_json() const {
	return Json()
	    ._("nodeType", "structTypeStaticMember")
	    ._("name", name)
	    ._("type", type->to_json())
	    ._("variability", variability)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

bool DefineStructType::is_generic() const { return not generics.empty(); }

void DefineStructType::setOpaque(ir::OpaqueType* oTy) const { opaquedTypes.push_back(oTy); }

bool DefineStructType::hasOpaque() const { return not opaquedTypes.empty(); }

ir::OpaqueType* DefineStructType::get_opaque() const { return opaquedTypes.back(); }

void DefineStructType::unsetOpaque() const { opaquedTypes.pop_back(); }

void DefineStructType::create_opaque(ir::Mod* mod, ir::Ctx* irCtx) {
	if (not is_generic()) {
		bool             hasAllMems = true;
		Vec<llvm::Type*> allMemEqTys;
		for (auto* mem : members) {
			auto memSize = mem->type->get_type_bitsize(EmitCtx::get(irCtx, mod));
			if (not memSize.has_value()) {
				hasAllMems = false;
				break;
			} else {
				allMemEqTys.push_back(llvm::Type::getIntNTy(irCtx->llctx, memSize.value()));
			}
		}
		Maybe<ir::MetaInfo> irMeta;
		bool                isTypeNatureUnion = false;
		if (metaInfo.has_value()) {
			irMeta = metaInfo.value().toIR(EmitCtx::get(irCtx, mod));
			if (irMeta->has_key(ir::MetaInfo::unionKey)) {
				if (not irMeta->has_key("foreign") && not mod->get_relevant_foreign_id().has_value()) {
					irCtx->Error(
					    "This type is not a foreign entity and is not part of any foreign module, and hence cannot be considered as a " +
					        irCtx->color(ir::MetaInfo::unionKey),
					    metaInfo.value().fileRange);
				}
				auto typeNatVal = irMeta->get_value_for(ir::MetaInfo::unionKey);
				if (not typeNatVal->get_ir_type()->is_bool()) {
					irCtx->Error("The key " + irCtx->color(ir::MetaInfo::unionKey) + " expects a value of type " +
					                 irCtx->color("bool"),
					             metaInfo.value().fileRange);
				}
				isTypeNatureUnion =
				    llvm::cast<llvm::ConstantInt>(typeNatVal->get_llvm_constant())->getValue().getBoolValue();
			}
			if (irMeta->has_key(ir::MetaInfo::packedKey)) {
				auto packVal = irMeta->get_value_for(ir::MetaInfo::packedKey);
				if (not packVal->get_ir_type()->is_bool()) {
					irCtx->Error("The key " + irCtx->color(ir::MetaInfo::packedKey) + " expects a value of type " +
					                 irCtx->color("bool"),
					             metaInfo.value().fileRange);
				}
				isPackedStruct = llvm::cast<llvm::ConstantInt>(packVal->get_llvm_constant())->getValue().getBoolValue();
			}
		}
		auto eqStructTy =
		    hasAllMems ? llvm::StructType::get(irCtx->llctx, allMemEqTys, isPackedStruct.value_or(false)) : nullptr;
		setOpaque(ir::OpaqueType::get(
		    name, {}, None, isTypeNatureUnion ? ir::OpaqueSubtypeKind::Union : ir::OpaqueSubtypeKind::STRUCT, mod,
		    eqStructTy ? Maybe<usize>(irCtx->dataLayout.getTypeAllocSizeInBits(eqStructTy)) : None,
		    EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx->llctx, irMeta));
	}
}

void DefineStructType::create_type_definitions(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx) {
	auto methodParent = ir::MethodParent::create_expanded_type(resultTy);
	auto emitCtx      = EmitCtx::get(irCtx, mod)->with_member_parent(methodParent);
	auto parentState  = get_state_for(methodParent);
	parentState->definitions.reserve(typeDefinitions.size());
	for (auto* def : typeDefinitions) {
		auto tyState = TypeInParentState{.isParentSkill = false, .parent = methodParent};
		def->create_type_in_parent(tyState, mod, irCtx);
		parentState->definitions.push_back(tyState);
	}
}

ir::StructType* DefineStructType::create_type(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod,
                                              ir::Ctx* irCtx) const {
	bool needsDestructor = false;
	auto cTyName         = name;
	SHOW("Creating IR generics")
	Vec<ir::GenericArgument*> genericsIR;
	for (auto* gen : generics) {
		if (not gen->isSet()) {
			if (gen->is_typed()) {
				irCtx->Error("No type is set for the generic type " + irCtx->color(gen->get_name().value) +
				                 " and there is no default type provided",
				             gen->get_range());
			} else if (gen->is_prerun()) {
				irCtx->Error("No value is set for the generic prerun expression " +
				                 irCtx->color(gen->get_name().value) + " and there is no default expression provided",
				             gen->get_range());
			} else {
				irCtx->Error("Invalid generic kind", gen->get_range());
			}
		}
		genericsIR.push_back(gen->toIRGenericType());
	}
	auto globalEmitCtx  = EmitCtx::get(irCtx, mod);
	auto mainVisibility = globalEmitCtx->get_visibility_info(visibSpec);
	if (is_generic()) {
		bool             hasAllMems = true;
		Vec<llvm::Type*> allMemEqTys;
		for (auto* mem : members) {
			auto memSize = mem->type->get_type_bitsize(EmitCtx::get(irCtx, mod));
			if (not memSize.has_value()) {
				hasAllMems = false;
				break;
			} else {
				allMemEqTys.push_back(llvm::Type::getIntNTy(irCtx->llctx, memSize.value()));
			}
		}
		Maybe<ir::MetaInfo> irMeta;
		if (metaInfo.has_value()) {
			irMeta = metaInfo.value().toIR(globalEmitCtx);
			if (irMeta->has_key(ir::MetaInfo::packedKey)) {
				auto packVal = irMeta->get_value_for(ir::MetaInfo::packedKey);
				if (not packVal->get_ir_type()->is_bool()) {
					irCtx->Error("The key " + irCtx->color(ir::MetaInfo::packedKey) + " expects a value of type " +
					                 irCtx->color("bool"),
					             metaInfo.value().fileRange);
				}
				isPackedStruct = llvm::cast<llvm::ConstantInt>(packVal->get_llvm_constant())->getValue().getBoolValue();
			}
		}
		auto eqStructTy =
		    hasAllMems ? llvm::StructType::get(irCtx->llctx, allMemEqTys, isPackedStruct.value_or(false)) : nullptr;
		SHOW("Setting opaque. Generic count: " << genericsIR.size() << " Module is " << mod << ". GenericStructType is "
		                                       << genericStructType)
		setOpaque(
		    ir::OpaqueType::get(cTyName, genericsIR, is_generic() ? Maybe<u64>(genericStructType->get_id()) : None,
		                        ir::OpaqueSubtypeKind::STRUCT, mod,
		                        eqStructTy ? Maybe<usize>(irCtx->dataLayout.getTypeAllocSizeInBits(eqStructTy)) : None,
		                        mainVisibility, irCtx->llctx, irMeta));
	}
	SHOW("Set opaque")
	if (genericStructType) {
		genericStructType->opaqueVariants.push_back(ir::GenericVariant<ir::OpaqueType>(get_opaque(), genericsToFill));
	}
	auto typeEmitCtx = EmitCtx::get(irCtx, mod)->with_opaque_parent(get_opaque());
	SHOW("Created opaque for struct type")
	Vec<ir::StructField*> mems;
	SHOW("Generating struct type members")
	for (auto* mem : members) {
		auto* memTy = mem->type->emit(typeEmitCtx);
		if (memTy->is_opaque() && not memTy->as_opaque()->has_subtype()) {
			// NOTE - Support sized opaques?
			if (memTy->is_same(get_opaque())) {
				irCtx->Error(
				    "Type nesting found. Member field " + irCtx->color(mem->name.value) + " is of type " +
				        irCtx->color(get_opaque()->to_string()) +
				        " which is the same as its parent type. Check the logic for mistakes or try using a pointer or a reference to the parent as the type instead",
				    mem->type->fileRange);
			} else {
				irCtx->Error("Member field " + irCtx->color(mem->name.value) + " has an incomplete type " +
				                 irCtx->color(memTy->to_string()) + " with an unknown size",
				             mem->type->fileRange);
			}
		}
		if (memTy->is_destructible()) {
			needsDestructor = true;
		}
		mems.push_back(ir::StructField::create(mem->name, memTy, mem->variability, mem->expression,
		                                       typeEmitCtx->get_visibility_info(mem->visibSpec)));
	}
	SHOW("Creating struct type: " << cTyName.value)
	auto resultType = ir::StructType::create(mod, cTyName, genericsIR, get_opaque(), mems, mainVisibility, irCtx->llctx,
	                                         None, isPackedStruct.value_or(false));
	if (genericStructType) {
		genericStructType->variants.push_back(ir::GenericVariant<ir::StructType>(resultType, genericsToFill));
	}
	SHOW("StructType ID: " << resultType->get_id())
	resultType->explicitSimpleCopy = simpleCopy.has_value();
	resultType->explicitSimpleMove = simpleMove.has_value();
	if (genericStructType) {
		for (auto item = genericStructType->opaqueVariants.begin(); item != genericStructType->opaqueVariants.end();
		     item++) {
			SHOW("Opaque variant: " << item->get())
			if (item->get()->get_id() == get_opaque()->get_id()) {
				genericStructType->opaqueVariants.erase(item);
				break;
			}
		}
	}
	unsetOpaque();
	SHOW("Unset opaque")
	if (destructorDefinition) {
		resultType->hasDefinedDestructor = true;
	}
	if (needsDestructor) {
		resultType->needsImplicitDestructor = true;
	}
	auto memParent = ir::MethodParent::create_expanded_type(resultType);
	auto emitCtx   = EmitCtx::get(irCtx, mod)->with_member_parent(memParent);
	for (auto* stm : staticMembers) {
		resultType->add_static_member(stm->name, stm->type->emit(emitCtx), stm->variability,
		                              stm->value ? stm->value->emit(emitCtx) : nullptr,
		                              emitCtx->get_visibility_info(stm->visibSpec), irCtx->llctx);
	}
	if (copyConstructor && not copyAssignment) {
		irCtx->Error("Copy constructor is defined for the type " + irCtx->color(resultType->to_string()) +
		                 ", and hence copy assignment operator is also required to be defined",
		             fileRange);
	}
	if (moveConstructor && not moveAssignment) {
		irCtx->Error("Move constructor is defined for the type " + irCtx->color(resultType->to_string()) +
		                 ", and hence move assignment operator is also required to be defined",
		             fileRange);
	}
	if (copyAssignment && not copyConstructor) {
		irCtx->Error("Copy assignment operator is defined for the type " + irCtx->color(resultType->to_string()) +
		                 ", and hence copy constructor is also required to be defined",
		             fileRange);
	}
	if (moveAssignment && not moveConstructor) {
		irCtx->Error("Move assignment operator is defined for the type " + irCtx->color(resultType->to_string()) +
		                 ", and hence move constructor is also required to be defined",
		             fileRange);
	}
	return resultType;
}

void DefineStructType::setup_type(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("Emitted generics")
	if (not is_generic()) {
		create_type({}, mod, irCtx);
	} else {
		auto emitCtx = EmitCtx::get(irCtx, mod);
		for (auto* gen : generics) {
			gen->emit(emitCtx);
		}
		genericStructType = ir::GenericStructType::create(name, generics, genericConstraint, this, mod,
		                                                  emitCtx->get_visibility_info(visibSpec));
	}
}

void DefineStructType::do_define(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx) {
	if (checkResult.has_value() && not checkResult.value()) {
		return;
	}
	auto memberParent = ir::MethodParent::create_expanded_type(resultTy);
	auto parentState  = get_state_for(memberParent);
	if (defaultConstructor) {
		defaultConstructor->define(parentState->defaultConstructor, irCtx);
	}
	if (copyConstructor) {
		copyConstructor->define(parentState->copyConstructor, irCtx);
	}
	if (moveConstructor) {
		moveConstructor->define(parentState->moveConstructor, irCtx);
	}
	if (copyAssignment) {
		copyAssignment->define(parentState->copyAssignment, irCtx);
	}
	if (moveAssignment) {
		moveAssignment->define(parentState->moveAssignment, irCtx);
	}
	if (destructorDefinition) {
		destructorDefinition->define(parentState->destructor, irCtx);
	}
	parentState->constructors.reserve(constructorDefinitions.size());
	parentState->convertors.reserve(convertorDefinitions.size());
	parentState->allMethods.reserve(methodDefinitions.size());
	parentState->operators.reserve(operatorDefinitions.size());
	for (auto* cons : constructorDefinitions) {
		MethodState state(memberParent);
		cons->define(state, irCtx);
		parentState->constructors.push_back(state);
	}
	for (auto* conv : convertorDefinitions) {
		MethodState state(memberParent);
		conv->define(state, irCtx);
		parentState->convertors.push_back(state);
	}
	for (auto* mFn : methodDefinitions) {
		MethodState state(memberParent);
		mFn->define(state, irCtx);
		parentState->allMethods.push_back(state);
	}
	for (auto* oFn : operatorDefinitions) {
		MethodState state(memberParent);
		oFn->define(state, irCtx);
		parentState->operators.push_back(state);
	}
}

void DefineStructType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	if (is_generic()) {
		mod->entity_name_check(irCtx, name, ir::EntityType::genericStructType);
		entityState = mod->add_entity(name, ir::EntityType::genericStructType, this, ir::EmitPhase::phase_1);
	} else {
		mod->entity_name_check(irCtx, name, ir::EntityType::structType);
		entityState                 = mod->add_entity(name, ir::EntityType::structType, this, ir::EmitPhase::phase_4);
		entityState->phaseToPartial = ir::EmitPhase::phase_1;
		entityState->phaseToCompletion      = ir::EmitPhase::phase_2;
		entityState->supportsChildren       = true;
		entityState->phaseToChildrenPartial = ir::EmitPhase::phase_3;
		for (auto memFn : methodDefinitions) {
			memFn->prototype->add_to_parent(entityState, irCtx);
		}
	}
}

void DefineStructType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (defineChecker != nullptr) {
		defineChecker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (genericConstraint != nullptr) {
		genericConstraint->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (is_generic()) {
		for (auto gen : generics) {
			gen->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
		}
	}
	for (auto mem : members) {
		mem->type->update_dependencies(is_generic() ? ir::EmitPhase::phase_1 : ir::EmitPhase::phase_2,
		                               ir::DependType::complete, entityState, ctx);
		if (mem->expression.has_value()) {
			mem->expression.value()->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState,
			                                             ctx);
		}
	}

	if (not is_generic()) {
		for (auto stat : staticMembers) {
			stat->type->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		}
		for (auto* def : typeDefinitions) {
			def->update_dependencies_for_parent(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		}
		if (defaultConstructor) {
			defaultConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete,
			                                                   entityState, ctx);
			defaultConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		if (copyConstructor) {
			copyConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete,
			                                                entityState, ctx);
			copyConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		if (moveConstructor) {
			moveConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete,
			                                                entityState, ctx);
			moveConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		if (copyAssignment) {
			copyAssignment->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete,
			                                               entityState, ctx);
			copyAssignment->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		if (moveAssignment) {
			moveAssignment->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete,
			                                               entityState, ctx);
			moveAssignment->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		if (destructorDefinition) {
			destructorDefinition->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState,
			                                          ctx);
		}
		for (auto cons : constructorDefinitions) {
			cons->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
			cons->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		for (auto conv : convertorDefinitions) {
			conv->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
			conv->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		for (auto memFn : methodDefinitions) {
			memFn->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
			memFn->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
		for (auto opr : operatorDefinitions) {
			opr->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
			opr->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
		}
	}
}

void DefineStructType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	if (checkResult.has_value() && not checkResult.value()) {
		return;
	} else if (defineChecker) {
		auto* checkRes = defineChecker->emit(EmitCtx::get(irCtx, mod));
		if (checkRes->get_ir_type()->is_bool()) {
			checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
			if (not checkResult.value()) {
				// TODO - ADD THIS AS DEAD CODE IN CODE INFO
				return;
			}
		} else {
			irCtx->Error("The condition for defining this struct type should be of " + irCtx->color("bool") +
			                 " type. Got an expression of type " + irCtx->color(checkRes->get_ir_type()->to_string()),
			             defineChecker->fileRange);
		}
	}
	if (is_generic()) {
		setup_type(mod, irCtx);
	} else {
		if (phase == ir::EmitPhase::phase_1) {
			create_opaque(mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_2) {
			resultType = create_type({}, mod, irCtx);
			create_type_definitions(resultType, mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_3) {
			do_define(resultType, mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_4) {
			do_emit(resultType, irCtx);
		}
	}
}

void DefineStructType::do_emit(ir::StructType* resultTy, ir::Ctx* irCtx) {
	SHOW("Emitting")
	if (checkResult.has_value() && not checkResult.value()) {
		return;
	}
	SHOW("Creating member parent")
	auto memberParent = ir::MethodParent::create_expanded_type(resultTy);
	auto parentState  = get_state_for(memberParent);
	SHOW("Got parent state")
	if (defaultConstructor) {
		(void)defaultConstructor->emit(parentState->defaultConstructor, irCtx);
	}
	SHOW("Default done")
	if (copyConstructor) {
		(void)copyConstructor->emit(parentState->copyConstructor, irCtx);
	}
	SHOW("Copy constr done")
	if (moveConstructor) {
		(void)moveConstructor->emit(parentState->moveConstructor, irCtx);
	}
	if (copyAssignment) {
		(void)copyAssignment->emit(parentState->copyAssignment, irCtx);
	}
	if (moveAssignment) {
		(void)moveAssignment->emit(parentState->moveAssignment, irCtx);
	}
	for (usize i = 0; i < constructorDefinitions.size(); i++) {
		(void)constructorDefinitions.at(i)->emit(parentState->constructors[i], irCtx);
	}
	for (usize i = 0; i < convertorDefinitions.size(); i++) {
		(void)convertorDefinitions[i]->emit(parentState->convertors[i], irCtx);
	}
	for (usize i = 0; i < methodDefinitions.size(); i++) {
		(void)methodDefinitions[i]->emit(parentState->allMethods[i], irCtx);
	}
	for (usize i = 0; i < operatorDefinitions.size(); i++) {
		(void)operatorDefinitions[i]->emit(parentState->operators[i], irCtx);
	}
	if (destructorDefinition) {
		(void)destructorDefinition->emit(parentState->destructor, irCtx);
	}
	// TODO - Member function call tree analysis
}

void DefineStructType::addMember(Member* mem) { members.push_back(mem); }

void DefineStructType::addStaticMember(StaticMember* stm) { staticMembers.push_back(stm); }

Json DefineStructType::to_json() const {
	Vec<JsonValue> membersJsonValue;
	Vec<JsonValue> staticMembersJsonValue;
	for (auto* mem : members) {
		membersJsonValue.emplace_back(mem->to_json());
	}
	for (auto* mem : staticMembers) {
		staticMembersJsonValue.emplace_back(mem->to_json());
	}
	return Json()
	    ._("nodeType", "defineStructType")
	    ._("members", membersJsonValue)
	    ._("staticMembers", staticMembersJsonValue)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

DefineStructType::~DefineStructType() {
	for (auto* mem : members) {
		std::destroy_at(mem);
	}
	for (auto* stat : staticMembers) {
		std::destroy_at(stat);
	}
}

} // namespace qat::ast
