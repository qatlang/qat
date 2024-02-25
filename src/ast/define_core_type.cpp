#include "./define_core_type.hpp"
#include "../IR/generics.hpp"
#include "../IR/types/core_type.hpp"
#include "../utils/identifier.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"
#include "constructor.hpp"
#include "convertor.hpp"
#include "destructor.hpp"
#include "member_function.hpp"
#include "member_parent_like.hpp"
#include "node.hpp"
#include "operator_function.hpp"
#include "types/generic_abstract.hpp"

namespace qat::ast {

Json DefineCoreType::Member::toJson() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

Json DefineCoreType::StaticMember::toJson() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

bool DefineCoreType::isGeneric() const { return !(generics.empty()); }

void DefineCoreType::setOpaque(IR::OpaqueType* oTy) const { opaquedTypes.push_back(oTy); }

bool DefineCoreType::hasOpaque() const {
  return false;
  !opaquedTypes.empty();
}

IR::OpaqueType* DefineCoreType::getOpaque() const { return opaquedTypes.back(); }

void DefineCoreType::unsetOpaque() const { opaquedTypes.pop_back(); }

void DefineCoreType::create_opaque(IR::QatModule* mod, IR::Context* ctx) {
  if (!isGeneric()) {
    bool             hasAllMems = true;
    Vec<llvm::Type*> allMemEqTys;
    for (auto* mem : members) {
      auto memSize = mem->type->getTypeSizeInBits(ctx);
      if (!memSize.has_value()) {
        hasAllMems = false;
        break;
      } else {
        allMemEqTys.push_back(llvm::Type::getIntNTy(ctx->llctx, memSize.value()));
      }
    }
    Maybe<IR::MetaInfo> irMeta;
    bool                isTypeNatureUnion = false;
    if (metaInfo.has_value()) {
      irMeta = metaInfo.value().toIR(ctx);
      if (irMeta->hasKey(IR::MetaInfo::unionKey)) {
        if (!irMeta->hasKey("foreign") && !mod->getRelevantForeignID().has_value()) {
          ctx->Error(
              "This type is not a foreign entity and is not part of any foreign module, and hence cannot be considered as a " +
                  ctx->highlightError(IR::MetaInfo::unionKey),
              metaInfo.value().fileRange);
        }
        auto typeNatVal = irMeta->getValueFor(IR::MetaInfo::unionKey);
        if (!typeNatVal->getType()->isBool()) {
          ctx->Error("The key " + ctx->highlightError(IR::MetaInfo::unionKey) + " expects a value of type " +
                         ctx->highlightError("bool"),
                     metaInfo.value().fileRange);
        }
        isTypeNatureUnion = llvm::cast<llvm::ConstantInt>(typeNatVal->getLLVMConstant())->getValue().getBoolValue();
      }
      if (irMeta->hasKey(IR::MetaInfo::packedKey)) {
        auto packVal = irMeta->getValueFor(IR::MetaInfo::packedKey);
        if (!packVal->getType()->isBool()) {
          ctx->Error("The key " + ctx->highlightError(IR::MetaInfo::packedKey) + " expects a value of type " +
                         ctx->highlightError("bool"),
                     metaInfo.value().fileRange);
        }
        isPackedStruct = llvm::cast<llvm::ConstantInt>(packVal->getLLVMConstant())->getValue().getBoolValue();
      }
    }
    auto eqStructTy =
        hasAllMems ? llvm::StructType::get(ctx->llctx, allMemEqTys, isPackedStruct.value_or(false)) : nullptr;
    auto mainVisibility = ctx->getVisibInfo(visibSpec);
    setOpaque(IR::OpaqueType::get(name, {}, None,
                                  isTypeNatureUnion ? IR::OpaqueSubtypeKind::Union : IR::OpaqueSubtypeKind::core, mod,
                                  eqStructTy ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
                                  mainVisibility, ctx->llctx, irMeta));
  }
}

void DefineCoreType::create_type(IR::CoreType** resultTy, IR::QatModule* mod, IR::Context* ctx) const {
  bool needsDestructor = false;
  auto cTyName         = name;
  SHOW("Creating IR generics")
  Vec<IR::GenericParameter*> genericsIR;
  for (auto* gen : generics) {
    if (!gen->isSet()) {
      if (gen->isTyped()) {
        ctx->Error("No type is set for the generic type " + ctx->highlightError(gen->getName().value) +
                       " and there is no default type provided",
                   gen->getRange());
      } else if (gen->isPrerun()) {
        ctx->Error("No value is set for the generic prerun expression " + ctx->highlightError(gen->getName().value) +
                       " and there is no default expression provided",
                   gen->getRange());
      } else {
        ctx->Error("Invalid generic kind", gen->getRange());
      }
    }
    genericsIR.push_back(gen->toIRGenericType());
  }
  auto mainVisibility = ctx->getVisibInfo(visibSpec);
  if (isGeneric()) {
    bool             hasAllMems = true;
    Vec<llvm::Type*> allMemEqTys;
    for (auto* mem : members) {
      auto memSize = mem->type->getTypeSizeInBits(ctx);
      if (!memSize.has_value()) {
        hasAllMems = false;
        break;
      } else {
        allMemEqTys.push_back(llvm::Type::getIntNTy(ctx->llctx, memSize.value()));
      }
    }
    Maybe<IR::MetaInfo> irMeta;
    if (metaInfo.has_value()) {
      irMeta = metaInfo.value().toIR(ctx);
      if (irMeta->hasKey(IR::MetaInfo::packedKey)) {
        auto packVal = irMeta->getValueFor(IR::MetaInfo::packedKey);
        if (!packVal->getType()->isBool()) {
          ctx->Error("The key " + ctx->highlightError(IR::MetaInfo::packedKey) + " expects a value of type " +
                         ctx->highlightError("bool"),
                     metaInfo.value().fileRange);
        }
        isPackedStruct = llvm::cast<llvm::ConstantInt>(packVal->getLLVMConstant())->getValue().getBoolValue();
      }
    }
    auto eqStructTy =
        hasAllMems ? llvm::StructType::get(ctx->llctx, allMemEqTys, isPackedStruct.value_or(false)) : nullptr;
    SHOW("Setting opaque. Generic count: " << genericsIR.size() << " Module is " << mod << ". GenericCoreType is "
                                           << genericCoreType << "; datalayout: " << ctx->dataLayout.has_value())
    setOpaque(IR::OpaqueType::get(cTyName, genericsIR, isGeneric() ? Maybe<String>(genericCoreType->getID()) : None,
                                  IR::OpaqueSubtypeKind::core, mod,
                                  eqStructTy ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
                                  mainVisibility, ctx->llctx, irMeta));
  }
  SHOW("Set opaque")
  if (genericCoreType) {
    genericCoreType->opaqueVariants.push_back(IR::GenericVariant<IR::OpaqueType>(getOpaque(), genericsToFill));
  }
  SHOW("Created opaque for core type")
  ctx->setActiveType(getOpaque());
  Vec<IR::CoreType::Member*> mems;
  SHOW("Generating core type members")
  for (auto* mem : members) {
    auto* memTy = mem->type->emit(ctx);
    if (memTy->isOpaque() && !memTy->asOpaque()->hasSubType()) {
      // NOTE - Support sized opaques?
      if (memTy->isSame(getOpaque())) {
        ctx->Error(
            "Type nesting found. Member field " + ctx->highlightError(mem->name.value) + " is of type " +
                ctx->highlightError(getOpaque()->toString()) +
                " which is the same as its parent type. Check the logic for mistakes or try using a pointer or a reference to the parent as the type instead",
            mem->type->fileRange);
      } else {
        ctx->Error("Member field " + ctx->highlightError(mem->name.value) + " has an incomplete type " +
                       ctx->highlightError(memTy->toString()) + " with an unknown size",
                   mem->type->fileRange);
      }
    }
    if (memTy->isDestructible()) {
      needsDestructor = true;
    }
    mems.push_back(new IR::CoreType::Member(mem->name, memTy, mem->variability, mem->expression,
                                            ctx->getVisibInfo(mem->visibSpec)));
  }
  SHOW("Creating core type: " << cTyName.value)
  *resultTy = new IR::CoreType(mod, cTyName, genericsIR, getOpaque(), mems, mainVisibility, ctx->llctx, None,
                               isPackedStruct.value_or(false));
  if (genericCoreType) {
    genericCoreType->variants.push_back(IR::GenericVariant<IR::CoreType>(*resultTy, genericsToFill));
  }
  SHOW("CoreType ID: " << (*resultTy)->getID())
  (*resultTy)->explicitTrivialCopy = trivialCopy.has_value();
  (*resultTy)->explicitTrivialMove = trivialMove.has_value();
  ctx->unsetActiveType();
  ctx->setActiveType(*resultTy);
  if (genericCoreType) {
    for (auto item = genericCoreType->opaqueVariants.begin(); item != genericCoreType->opaqueVariants.end(); item++) {
      SHOW("Opaque variant: " << item->get())
      if (item->get()->getID() == getOpaque()->getID()) {
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
  for (auto* stm : staticMembers) {
    (*resultTy)->addStaticMember(stm->name, stm->type->emit(ctx), stm->variability,
                                 stm->value ? stm->value->emit(ctx) : nullptr, ctx->getVisibInfo(stm->visibSpec),
                                 ctx->llctx);
  }
  if (copyConstructor && !copyAssignment) {
    ctx->Error("Copy constructor is defined for the type " + ctx->highlightError((*resultTy)->toString()) +
                   ", and hence copy assignment operator is also required to be defined",
               fileRange);
  }
  if (moveConstructor && !moveAssignment) {
    ctx->Error("Move constructor is defined for the type " + ctx->highlightError((*resultTy)->toString()) +
                   ", and hence move assignment operator is also required to be defined",
               fileRange);
  }
  if (copyAssignment && !copyConstructor) {
    ctx->Error("Copy assignment operator is defined for the type " + ctx->highlightError((*resultTy)->toString()) +
                   ", and hence copy constructor is also required to be defined",
               fileRange);
  }
  if (moveAssignment && !moveConstructor) {
    ctx->Error("Move assignment operator is defined for the type " + ctx->highlightError((*resultTy)->toString()) +
                   ", and hence move constructor is also required to be defined",
               fileRange);
  }
  ctx->unsetActiveType();
}

void DefineCoreType::setup_type(IR::QatModule* mod, IR::Context* ctx) {
  for (auto* gen : generics) {
    SHOW("Generic parameter is " << gen->getName().value)
    gen->emit(ctx);
    SHOW("Emit complete for parameter")
  }
  SHOW("Emitted generics")
  if (!isGeneric()) {
    create_type(&resultCoreType, mod, ctx);
  } else {
    genericCoreType = new IR::GenericCoreType(name, generics, constraint, this, mod, ctx->getVisibInfo(visibSpec));
  }
  SHOW("Set old mod")
}

void DefineCoreType::do_define(IR::CoreType* resultTy, IR::QatModule* mod, IR::Context* ctx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  auto memberParent = IR::MemberParent::create_expanded_type(resultTy);
  auto parentState  = get_state_for(memberParent);
  if (defaultConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    defaultConstructor->define(state, ctx);
    parentState->defaultConstructor = state.result;
    ctx->unsetActiveType();
  }
  if (copyConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    copyConstructor->define(state, ctx);
    parentState->copyConstructor = state.result;
    ctx->unsetActiveType();
  }
  if (moveConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    moveConstructor->define(state, ctx);
    parentState->moveConstructor = state.result;
    ctx->unsetActiveType();
  }
  if (copyAssignment) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    copyAssignment->define(state, ctx);
    parentState->copyAssignment = state.result;
    ctx->unsetActiveType();
  }
  if (moveAssignment) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    moveAssignment->define(state, ctx);
    parentState->moveAssignment = state.result;
    ctx->unsetActiveType();
  }
  if (destructorDefinition) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    destructorDefinition->define(state, ctx);
    parentState->destructor = state.result;
    ctx->unsetActiveType();
  }
  for (auto* cons : constructorDefinitions) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    cons->define(state, ctx);
    parentState->constructors.push_back(state.result);
    ctx->unsetActiveType();
  }
  for (auto& conv : convertorDefinitions) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    conv->define(state, ctx);
    parentState->convertors.push_back(state.result);
    ctx->unsetActiveType();
  }
  for (auto* mFn : memberDefinitions) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    mFn->define(state, ctx);
    SHOW("Method done result is " << state.result << " defineCond: " << state.defineCondition.has_value())
    SHOW("All methods size: " << parentState->all_methods.size() << " for "
                              << memberParent->getParentType()->toString())
    parentState->all_methods.push_back(MethodResult(state.result, state.defineCondition));
    SHOW("Updated parent state")
    ctx->unsetActiveType();
  }
  SHOW("All methods done")
  for (auto* oFn : operatorDefinitions) {
    SHOW("Handling opr")
    ctx->setActiveType(resultTy);
    MethodState state(memberParent);
    oFn->define(state, ctx);
    SHOW("Defined operator")
    parentState->operators.push_back(state.result);
    ctx->unsetActiveType();
    SHOW("Operator complete")
  }
}

void DefineCoreType::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(ctx, name, IR::EntityType::structType);
  entityState = mod->add_entity(name, isGeneric() ? IR::EntityType::genericStructType : IR::EntityType::structType,
                                this, isGeneric() ? IR::EmitPhase::phase_1 : IR::EmitPhase::phase_4);
  if (!isGeneric()) {
    entityState->phaseToPartial         = IR::EmitPhase::phase_1;
    entityState->phaseToCompletion      = IR::EmitPhase::phase_2;
    entityState->supportsChildren       = true;
    entityState->phaseToChildrenPartial = IR::EmitPhase::phase_3;
    for (auto memFn : memberDefinitions) {
      memFn->prototype->add_to_parent(entityState, ctx);
    }
  }
}

void DefineCoreType::update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) {
  if (checker.has_value()) {
    checker.value()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
  }
  if (isGeneric()) {
    for (auto gen : generics) {
      gen->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
    }
  }
  for (auto mem : members) {
    mem->type->update_dependencies(isGeneric() ? IR::EmitPhase::phase_1 : IR::EmitPhase::phase_2,
                                   IR::DependType::complete, entityState, ctx);
    if (mem->expression.has_value()) {
      mem->expression.value()->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
  }
  for (auto stat : staticMembers) {
    stat->type->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
  }
  if (!isGeneric()) {
    if (defaultConstructor) {
      defaultConstructor->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState,
                                                         ctx);
      defaultConstructor->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    if (copyConstructor) {
      copyConstructor->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState,
                                                      ctx);
      copyConstructor->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    if (moveConstructor) {
      moveConstructor->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState,
                                                      ctx);
      moveConstructor->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    if (copyAssignment) {
      copyAssignment->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState,
                                                     ctx);
      copyAssignment->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    if (moveAssignment) {
      moveAssignment->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState,
                                                     ctx);
      moveAssignment->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    if (destructorDefinition) {
      destructorDefinition->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    for (auto cons : constructorDefinitions) {
      cons->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
      cons->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    for (auto conv : convertorDefinitions) {
      conv->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
      conv->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    for (auto memFn : memberDefinitions) {
      memFn->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
      memFn->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
    for (auto opr : operatorDefinitions) {
      opr->prototype->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
      opr->update_dependencies(IR::EmitPhase::phase_4, IR::DependType::complete, entityState, ctx);
    }
  }
}

void DefineCoreType::do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) {
  if (checker.has_value()) {
    auto* checkRes = checker.value()->emit(ctx);
    if (checkRes->getType()->isBool()) {
      checkResult = llvm::cast<llvm::ConstantInt>(checkRes->getLLVMConstant())->getValue().getBoolValue();
      if (!checkResult.value()) {
        // TODO - ADD THIS AS DEAD CODE IN CODE INFO
        return;
      }
    } else {
      ctx->Error("The condition for defining this struct type should be of " + ctx->highlightError("bool") +
                     " type. Got an expression of type " + ctx->highlightError(checkRes->getType()->toString()),
                 checker.value()->fileRange);
    }
  }
  if (isGeneric()) {
    setup_type(mod, ctx);
  } else {
    if (phase == IR::EmitPhase::phase_1) {
      create_opaque(mod, ctx);
    } else if (phase == IR::EmitPhase::phase_2) {
      create_type(&resultCoreType, mod, ctx);
    } else if (phase == IR::EmitPhase::phase_3) {
      do_define(resultCoreType, mod, ctx);
    } else if (phase == IR::EmitPhase::phase_4) {
      do_emit(resultCoreType, ctx);
    }
  }
}

void DefineCoreType::do_emit(IR::CoreType* resultTy, IR::Context* ctx) {
  SHOW("Emitting")
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  SHOW("Creating member parent")
  auto memberParent = IR::MemberParent::create_expanded_type(resultTy);
  auto parentState  = get_state_for(memberParent);
  SHOW("Got parent state")
  if (defaultConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->defaultConstructor);
    SHOW("Created method state, emitting")
    (void)defaultConstructor->emit(state, ctx);
    SHOW("EMit complete")
    ctx->unsetActiveType();
  }
  SHOW("Default done")
  if (copyConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->copyConstructor);
    (void)copyConstructor->emit(state, ctx);
    ctx->unsetActiveType();
  }
  SHOW("Copy constr done")
  if (moveConstructor) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->moveConstructor);
    (void)moveConstructor->emit(state, ctx);
    ctx->unsetActiveType();
  }
  if (copyAssignment) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->copyAssignment);
    (void)copyAssignment->emit(state, ctx);
    ctx->unsetActiveType();
  }
  if (moveAssignment) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->moveAssignment);
    (void)moveAssignment->emit(state, ctx);
    ctx->unsetActiveType();
  }
  for (usize i = 0; i < constructorDefinitions.size(); i++) {
    ctx->setActiveType(resultTy);
    SHOW("Constructor at " << i << " is " << parentState->constructors.at(i))
    MethodState state(memberParent, parentState->constructors.at(i));
    (void)constructorDefinitions.at(i)->emit(state, ctx);
    ctx->unsetActiveType();
  }
  for (usize i = 0; i < convertorDefinitions.size(); i++) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->convertors[i]);
    (void)convertorDefinitions[i]->emit(state, ctx);
    ctx->unsetActiveType();
  }
  for (usize i = 0; i < memberDefinitions.size(); i++) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->all_methods.at(i).fn, parentState->all_methods.at(i).condition);
    (void)memberDefinitions[i]->emit(state, ctx);
    ctx->unsetActiveType();
  }
  for (usize i = 0; i < operatorDefinitions.size(); i++) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->operators.at(i));
    (void)operatorDefinitions[i]->emit(state, ctx);
    ctx->unsetActiveType();
  }
  if (destructorDefinition) {
    ctx->setActiveType(resultTy);
    MethodState state(memberParent, parentState->destructor);
    (void)destructorDefinition->emit(state, ctx);
    ctx->unsetActiveType();
  }
  // TODO - Member function call tree analysis
}

void DefineCoreType::emit(IR::Context* ctx) { return do_emit(resultCoreType, ctx); }

void DefineCoreType::addMember(Member* mem) { members.push_back(mem); }

void DefineCoreType::addStaticMember(StaticMember* stm) { staticMembers.push_back(stm); }

Json DefineCoreType::toJson() const {
  Vec<JsonValue> membersJsonValue;
  Vec<JsonValue> staticMembersJsonValue;
  for (auto* mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  for (auto* mem : staticMembers) {
    staticMembersJsonValue.emplace_back(mem->toJson());
  }
  return Json()
      ._("nodeType", "defineCoreType")
      ._("members", membersJsonValue)
      ._("staticMembers", staticMembersJsonValue)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
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