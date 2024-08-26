#include "./define_core_type.hpp"
#include "../IR/generics.hpp"
#include "../IR/types/core_type.hpp"
#include "../utils/identifier.hpp"
#include "constructor.hpp"
#include "convertor.hpp"
#include "destructor.hpp"
#include "member_function.hpp"
#include "member_parent_like.hpp"
#include "node.hpp"
#include "operator_function.hpp"
#include "types/generic_abstract.hpp"

namespace qat::ast {

Json DefineCoreType::Member::to_json() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->to_json())
      ._("variability", variability)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("fileRange", fileRange);
}

Json DefineCoreType::StaticMember::to_json() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->to_json())
      ._("variability", variability)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("fileRange", fileRange);
}

bool DefineCoreType::isGeneric() const { return !(generics.empty()); }

void DefineCoreType::setOpaque(ir::OpaqueType* oTy) const { opaquedTypes.push_back(oTy); }

bool DefineCoreType::hasOpaque() const {
  return false;
  !opaquedTypes.empty();
}

ir::OpaqueType* DefineCoreType::get_opaque() const { return opaquedTypes.back(); }

void DefineCoreType::unsetOpaque() const { opaquedTypes.pop_back(); }

void DefineCoreType::create_opaque(ir::Mod* mod, ir::Ctx* irCtx) {
  if (!isGeneric()) {
    bool             hasAllMems = true;
    Vec<llvm::Type*> allMemEqTys;
    for (auto* mem : members) {
      auto memSize = mem->type->getTypeSizeInBits(EmitCtx::get(irCtx, mod));
      if (!memSize.has_value()) {
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
        if (!irMeta->has_key("foreign") && !mod->get_relevant_foreign_id().has_value()) {
          irCtx->Error(
              "This type is not a foreign entity and is not part of any foreign module, and hence cannot be considered as a " +
                  irCtx->color(ir::MetaInfo::unionKey),
              metaInfo.value().fileRange);
        }
        auto typeNatVal = irMeta->get_value_for(ir::MetaInfo::unionKey);
        if (!typeNatVal->get_ir_type()->is_bool()) {
          irCtx->Error("The key " + irCtx->color(ir::MetaInfo::unionKey) + " expects a value of type " +
                           irCtx->color("bool"),
                       metaInfo.value().fileRange);
        }
        isTypeNatureUnion = llvm::cast<llvm::ConstantInt>(typeNatVal->get_llvm_constant())->getValue().getBoolValue();
      }
      if (irMeta->has_key(ir::MetaInfo::packedKey)) {
        auto packVal = irMeta->get_value_for(ir::MetaInfo::packedKey);
        if (!packVal->get_ir_type()->is_bool()) {
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
        name, {}, None, isTypeNatureUnion ? ir::OpaqueSubtypeKind::Union : ir::OpaqueSubtypeKind::core, mod,
        eqStructTy ? Maybe<usize>(irCtx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
        EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx->llctx, irMeta));
  }
}

void DefineCoreType::create_type(ir::StructType** resultTy, ir::Mod* mod, ir::Ctx* irCtx) const {
  bool needsDestructor = false;
  auto cTyName         = name;
  SHOW("Creating IR generics")
  Vec<ir::GenericParameter*> genericsIR;
  for (auto* gen : generics) {
    if (!gen->isSet()) {
      if (gen->is_typed()) {
        irCtx->Error("No type is set for the generic type " + irCtx->color(gen->get_name().value) +
                         " and there is no default type provided",
                     gen->get_range());
      } else if (gen->is_prerun()) {
        irCtx->Error("No value is set for the generic prerun expression " + irCtx->color(gen->get_name().value) +
                         " and there is no default expression provided",
                     gen->get_range());
      } else {
        irCtx->Error("Invalid generic kind", gen->get_range());
      }
    }
    genericsIR.push_back(gen->toIRGenericType());
  }
  auto globalEmitCtx  = EmitCtx::get(irCtx, mod);
  auto mainVisibility = globalEmitCtx->get_visibility_info(visibSpec);
  if (isGeneric()) {
    bool             hasAllMems = true;
    Vec<llvm::Type*> allMemEqTys;
    for (auto* mem : members) {
      auto memSize = mem->type->getTypeSizeInBits(EmitCtx::get(irCtx, mod));
      if (!memSize.has_value()) {
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
        if (!packVal->get_ir_type()->is_bool()) {
          irCtx->Error("The key " + irCtx->color(ir::MetaInfo::packedKey) + " expects a value of type " +
                           irCtx->color("bool"),
                       metaInfo.value().fileRange);
        }
        isPackedStruct = llvm::cast<llvm::ConstantInt>(packVal->get_llvm_constant())->getValue().getBoolValue();
      }
    }
    auto eqStructTy =
        hasAllMems ? llvm::StructType::get(irCtx->llctx, allMemEqTys, isPackedStruct.value_or(false)) : nullptr;
    SHOW("Setting opaque. Generic count: " << genericsIR.size() << " Module is " << mod << ". GenericCoreType is "
                                           << genericCoreType << "; datalayout: " << irCtx->dataLayout.has_value())
    setOpaque(ir::OpaqueType::get(
        cTyName, genericsIR, isGeneric() ? Maybe<String>(genericCoreType->get_id()) : None, ir::OpaqueSubtypeKind::core,
        mod, eqStructTy ? Maybe<usize>(irCtx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None, mainVisibility,
        irCtx->llctx, irMeta));
  }
  SHOW("Set opaque")
  if (genericCoreType) {
    genericCoreType->opaqueVariants.push_back(ir::GenericVariant<ir::OpaqueType>(get_opaque(), genericsToFill));
  }
  auto typeEmitCtx = EmitCtx::get(irCtx, mod)->with_opaque_parent(get_opaque());
  SHOW("Created opaque for core type")
  Vec<ir::StructType::Member*> mems;
  SHOW("Generating core type members")
  for (auto* mem : members) {
    auto* memTy = mem->type->emit(typeEmitCtx);
    if (memTy->is_opaque() && !memTy->as_opaque()->has_subtype()) {
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
    mems.push_back(new ir::StructType::Member(mem->name, memTy, mem->variability, mem->expression,
                                              typeEmitCtx->get_visibility_info(mem->visibSpec)));
  }
  SHOW("Creating core type: " << cTyName.value)
  *resultTy = new ir::StructType(mod, cTyName, genericsIR, get_opaque(), mems, mainVisibility, irCtx->llctx, None,
                                 isPackedStruct.value_or(false));
  if (genericCoreType) {
    genericCoreType->variants.push_back(ir::GenericVariant<ir::StructType>(*resultTy, genericsToFill));
  }
  SHOW("StructType ID: " << (*resultTy)->get_id())
  (*resultTy)->explicitTrivialCopy = trivialCopy.has_value();
  (*resultTy)->explicitTrivialMove = trivialMove.has_value();
  if (genericCoreType) {
    for (auto item = genericCoreType->opaqueVariants.begin(); item != genericCoreType->opaqueVariants.end(); item++) {
      SHOW("Opaque variant: " << item->get())
      if (item->get()->get_id() == get_opaque()->get_id()) {
        genericCoreType->opaqueVariants.erase(item);
        break;
      }
    }
  }
  unsetOpaque();
  SHOW("Unset opaque")
  if (destructorDefinition) {
    (*resultTy)->hasDefinedDestructor = true;
  }
  if (needsDestructor) {
    (*resultTy)->needsImplicitDestructor = true;
  }
  auto memParent = ir::MethodParent::create_expanded_type(*resultTy);
  auto emitCtx   = EmitCtx::get(irCtx, mod)->with_member_parent(memParent);
  for (auto* stm : staticMembers) {
    (*resultTy)->addStaticMember(stm->name, stm->type->emit(emitCtx), stm->variability,
                                 stm->value ? stm->value->emit(emitCtx) : nullptr,
                                 emitCtx->get_visibility_info(stm->visibSpec), irCtx->llctx);
  }
  if (copyConstructor && !copyAssignment) {
    irCtx->Error("Copy constructor is defined for the type " + irCtx->color((*resultTy)->to_string()) +
                     ", and hence copy assignment operator is also required to be defined",
                 fileRange);
  }
  if (moveConstructor && !moveAssignment) {
    irCtx->Error("Move constructor is defined for the type " + irCtx->color((*resultTy)->to_string()) +
                     ", and hence move assignment operator is also required to be defined",
                 fileRange);
  }
  if (copyAssignment && !copyConstructor) {
    irCtx->Error("Copy assignment operator is defined for the type " + irCtx->color((*resultTy)->to_string()) +
                     ", and hence copy constructor is also required to be defined",
                 fileRange);
  }
  if (moveAssignment && !moveConstructor) {
    irCtx->Error("Move assignment operator is defined for the type " + irCtx->color((*resultTy)->to_string()) +
                     ", and hence move constructor is also required to be defined",
                 fileRange);
  }
}

void DefineCoreType::setup_type(ir::Mod* mod, ir::Ctx* irCtx) {
  SHOW("Emitted generics")
  if (!isGeneric()) {
    create_type(&resultCoreType, mod, irCtx);
  } else {
    auto emitCtx = EmitCtx::get(irCtx, mod);
    for (auto* gen : generics) {
      gen->emit(emitCtx);
    }
    genericCoreType =
        new ir::GenericCoreType(name, generics, constraint, this, mod, emitCtx->get_visibility_info(visibSpec));
  }
}

void DefineCoreType::do_define(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  auto memberParent = ir::MethodParent::create_expanded_type(resultTy);
  auto parentState  = get_state_for(memberParent);
  if (defaultConstructor) {
    MethodState state(memberParent);
    defaultConstructor->define(state, irCtx);
    parentState->defaultConstructor = state.result;
  }
  if (copyConstructor) {
    MethodState state(memberParent);
    copyConstructor->define(state, irCtx);
    parentState->copyConstructor = state.result;
  }
  if (moveConstructor) {
    MethodState state(memberParent);
    moveConstructor->define(state, irCtx);
    parentState->moveConstructor = state.result;
  }
  if (copyAssignment) {
    MethodState state(memberParent);
    copyAssignment->define(state, irCtx);
    parentState->copyAssignment = state.result;
  }
  if (moveAssignment) {
    MethodState state(memberParent);
    moveAssignment->define(state, irCtx);
    parentState->moveAssignment = state.result;
  }
  if (destructorDefinition) {
    MethodState state(memberParent);
    destructorDefinition->define(state, irCtx);
    parentState->destructor = state.result;
  }
  for (auto* cons : constructorDefinitions) {
    MethodState state(memberParent);
    cons->define(state, irCtx);
    parentState->constructors.push_back(state.result);
  }
  for (auto& conv : convertorDefinitions) {
    MethodState state(memberParent);
    conv->define(state, irCtx);
    parentState->convertors.push_back(state.result);
  }
  for (auto* mFn : memberDefinitions) {
    MethodState state(memberParent);
    mFn->define(state, irCtx);
    parentState->all_methods.push_back(MethodResult(state.result, state.defineCondition));
  }
  SHOW("All methods done")
  for (auto* oFn : operatorDefinitions) {
    MethodState state(memberParent);
    oFn->define(state, irCtx);
    parentState->operators.push_back(state.result);
  }
}

void DefineCoreType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(irCtx, name, ir::EntityType::structType);
  entityState = mod->add_entity(name, isGeneric() ? ir::EntityType::genericStructType : ir::EntityType::structType,
                                this, isGeneric() ? ir::EmitPhase::phase_1 : ir::EmitPhase::phase_4);
  if (!isGeneric()) {
    entityState->phaseToPartial         = ir::EmitPhase::phase_1;
    entityState->phaseToCompletion      = ir::EmitPhase::phase_2;
    entityState->supportsChildren       = true;
    entityState->phaseToChildrenPartial = ir::EmitPhase::phase_3;
    for (auto memFn : memberDefinitions) {
      memFn->prototype->add_to_parent(entityState, irCtx);
    }
  }
}

void DefineCoreType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
  auto ctx = EmitCtx::get(irCtx, mod);
  if (checker.has_value()) {
    checker.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
  }
  if (isGeneric()) {
    for (auto gen : generics) {
      gen->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
    }
  }
  for (auto mem : members) {
    mem->type->update_dependencies(isGeneric() ? ir::EmitPhase::phase_1 : ir::EmitPhase::phase_2,
                                   ir::DependType::complete, entityState, ctx);
    if (mem->expression.has_value()) {
      mem->expression.value()->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
  }
  for (auto stat : staticMembers) {
    stat->type->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
  }
  if (!isGeneric()) {
    if (defaultConstructor) {
      defaultConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
                                                         ctx);
      defaultConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    if (copyConstructor) {
      copyConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
                                                      ctx);
      copyConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    if (moveConstructor) {
      moveConstructor->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
                                                      ctx);
      moveConstructor->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    if (copyAssignment) {
      copyAssignment->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
                                                     ctx);
      copyAssignment->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    if (moveAssignment) {
      moveAssignment->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
                                                     ctx);
      moveAssignment->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    if (destructorDefinition) {
      destructorDefinition->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    for (auto cons : constructorDefinitions) {
      cons->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
      cons->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    for (auto conv : convertorDefinitions) {
      conv->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
      conv->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    for (auto memFn : memberDefinitions) {
      memFn->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
      memFn->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
    for (auto opr : operatorDefinitions) {
      opr->prototype->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
      opr->update_dependencies(ir::EmitPhase::phase_4, ir::DependType::complete, entityState, ctx);
    }
  }
}

void DefineCoreType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
  if (checker.has_value()) {
    auto* checkRes = checker.value()->emit(EmitCtx::get(irCtx, mod));
    if (checkRes->get_ir_type()->is_bool()) {
      checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
      if (!checkResult.value()) {
        // TODO - ADD THIS AS DEAD CODE IN CODE INFO
        return;
      }
    } else {
      irCtx->Error("The condition for defining this struct type should be of " + irCtx->color("bool") +
                       " type. Got an expression of type " + irCtx->color(checkRes->get_ir_type()->to_string()),
                   checker.value()->fileRange);
    }
  }
  if (isGeneric()) {
    setup_type(mod, irCtx);
  } else {
    if (phase == ir::EmitPhase::phase_1) {
      create_opaque(mod, irCtx);
    } else if (phase == ir::EmitPhase::phase_2) {
      create_type(&resultCoreType, mod, irCtx);
    } else if (phase == ir::EmitPhase::phase_3) {
      do_define(resultCoreType, mod, irCtx);
    } else if (phase == ir::EmitPhase::phase_4) {
      do_emit(resultCoreType, irCtx);
    }
  }
}

void DefineCoreType::do_emit(ir::StructType* resultTy, ir::Ctx* irCtx) {
  SHOW("Emitting")
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  SHOW("Creating member parent")
  auto memberParent = ir::MethodParent::create_expanded_type(resultTy);
  auto parentState  = get_state_for(memberParent);
  SHOW("Got parent state")
  if (defaultConstructor) {
    MethodState state(memberParent, parentState->defaultConstructor);
    (void)defaultConstructor->emit(state, irCtx);
  }
  SHOW("Default done")
  if (copyConstructor) {
    MethodState state(memberParent, parentState->copyConstructor);
    (void)copyConstructor->emit(state, irCtx);
  }
  SHOW("Copy constr done")
  if (moveConstructor) {
    MethodState state(memberParent, parentState->moveConstructor);
    (void)moveConstructor->emit(state, irCtx);
  }
  if (copyAssignment) {
    MethodState state(memberParent, parentState->copyAssignment);
    (void)copyAssignment->emit(state, irCtx);
  }
  if (moveAssignment) {
    MethodState state(memberParent, parentState->moveAssignment);
    (void)moveAssignment->emit(state, irCtx);
  }
  for (usize i = 0; i < constructorDefinitions.size(); i++) {
    MethodState state(memberParent, parentState->constructors.at(i));
    (void)constructorDefinitions.at(i)->emit(state, irCtx);
  }
  for (usize i = 0; i < convertorDefinitions.size(); i++) {
    MethodState state(memberParent, parentState->convertors[i]);
    (void)convertorDefinitions[i]->emit(state, irCtx);
  }
  for (usize i = 0; i < memberDefinitions.size(); i++) {
    MethodState state(memberParent, parentState->all_methods.at(i).fn, parentState->all_methods.at(i).condition);
    (void)memberDefinitions[i]->emit(state, irCtx);
  }
  for (usize i = 0; i < operatorDefinitions.size(); i++) {
    MethodState state(memberParent, parentState->operators.at(i));
    (void)operatorDefinitions[i]->emit(state, irCtx);
  }
  if (destructorDefinition) {
    MethodState state(memberParent, parentState->destructor);
    (void)destructorDefinition->emit(state, irCtx);
  }
  // TODO - Member function call tree analysis
}

void DefineCoreType::emit(ir::Ctx* irCtx) { return do_emit(resultCoreType, irCtx); }

void DefineCoreType::addMember(Member* mem) { members.push_back(mem); }

void DefineCoreType::addStaticMember(StaticMember* stm) { staticMembers.push_back(stm); }

Json DefineCoreType::to_json() const {
  Vec<JsonValue> membersJsonValue;
  Vec<JsonValue> staticMembersJsonValue;
  for (auto* mem : members) {
    membersJsonValue.emplace_back(mem->to_json());
  }
  for (auto* mem : staticMembers) {
    staticMembersJsonValue.emplace_back(mem->to_json());
  }
  return Json()
      ._("nodeType", "defineCoreType")
      ._("members", membersJsonValue)
      ._("staticMembers", staticMembersJsonValue)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("fileRange", fileRange);
}

DefineCoreType::~DefineCoreType() {
  for (auto* mem : members) {
    std::destroy_at(mem);
  }
  for (auto* stat : staticMembers) {
    std::destroy_at(stat);
  }
}

} // namespace qat::ast