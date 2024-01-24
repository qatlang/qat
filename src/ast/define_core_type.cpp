#include "./define_core_type.hpp"
#include "../IR/generics.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/core_type.hpp"
#include "../utils/identifier.hpp"
#include "constructor.hpp"
#include "destructor.hpp"
#include "node.hpp"
#include "types/generic_abstract.hpp"
#include <algorithm>

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

void DefineCoreType::setCoreType(IR::CoreType* cTy) const { coreTypes.push_back(cTy); }

void DefineCoreType::unsetCoreType() const { coreTypes.pop_back(); }

void DefineCoreType::setOpaque(IR::OpaqueType* oTy) const { opaquedTypes.push_back(oTy); }

bool DefineCoreType::hasOpaque() const {
  return false;
  !opaquedTypes.empty();
}

IR::OpaqueType* DefineCoreType::getOpaque() const { return opaquedTypes.back(); }

void DefineCoreType::unsetOpaque() const { opaquedTypes.pop_back(); }

void DefineCoreType::createModule(IR::Context* ctx) const {
  if (!isGeneric()) {
    auto* mod = ctx->getMod();
    ctx->nameCheckInModule(name, "core type", None);
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
    auto eqStructTy     = hasAllMems ? llvm::StructType::get(ctx->llctx, allMemEqTys, isPacked) : nullptr;
    auto mainVisibility = ctx->getVisibInfo(visibSpec);
    setOpaque(IR::OpaqueType::get(name, {}, None, IR::OpaqueSubtypeKind::core, mod,
                                  eqStructTy ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
                                  mainVisibility, ctx->llctx, None));
  }
}

void DefineCoreType::createType(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, isGeneric() ? "generic core type" : "core type",
                         isGeneric() ? Maybe<String>(genericCoreType->getID()) : None,
                         isGeneric() ? None : Maybe<String>(getOpaque()->getID()));
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
    auto eqStructTy = hasAllMems ? llvm::StructType::get(ctx->llctx, allMemEqTys, isPacked) : nullptr;
    SHOW("Setting opaque. Generic count: " << genericsIR.size() << " Module is " << mod << ". GenericCoreType is "
                                           << genericCoreType << "; datalayout: " << ctx->dataLayout.has_value())
    setOpaque(IR::OpaqueType::get(cTyName, genericsIR, isGeneric() ? Maybe<String>(genericCoreType->getID()) : None,
                                  IR::OpaqueSubtypeKind::core, mod,
                                  eqStructTy ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
                                  mainVisibility, ctx->llctx, None));
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
  setCoreType(new IR::CoreType(mod, cTyName, genericsIR, getOpaque(), mems, mainVisibility, ctx->llctx, None));
  if (genericCoreType) {
    genericCoreType->variants.push_back(IR::GenericVariant<IR::CoreType>(getCoreType(), genericsToFill));
  }
  SHOW("CoreType ID: " << coreTypes.back()->getID())
  getCoreType()->explicitTrivialCopy = trivialCopy.has_value();
  getCoreType()->explicitTrivialMove = trivialMove.has_value();
  ctx->unsetActiveType();
  ctx->setActiveType(getCoreType());
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
  if (destructorDefinition) {
    getCoreType()->hasDefinedDestructor = true;
  }
  if (needsDestructor) {
    getCoreType()->needsImplicitDestructor = true;
  }
  for (auto* stm : staticMembers) {
    getCoreType()->addStaticMember(stm->name, stm->type->emit(ctx), stm->variability,
                                   stm->value ? stm->value->emit(ctx) : nullptr, ctx->getVisibInfo(stm->visibSpec),
                                   ctx->llctx);
  }
  if (defaultConstructor) {
    defaultConstructor->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  if (copyConstructor) {
    if (!copyAssignment) {
      ctx->Error("Copy constructor is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence copy assignment operator is also required to be defined",
                 fileRange);
    }
    copyConstructor->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  if (moveConstructor) {
    if (!moveAssignment) {
      ctx->Error("Move constructor is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence move assignment operator is also required to be defined",
                 fileRange);
    }
    moveConstructor->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  if (copyAssignment) {
    if (!copyConstructor) {
      ctx->Error("Copy assignment operator is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence copy constructor is also required to be defined",
                 fileRange);
    }
    copyAssignment->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  if (moveAssignment) {
    if (!moveConstructor) {
      ctx->Error("Move assignment operator is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence move constructor is also required to be defined",
                 fileRange);
    }
    moveAssignment->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  if (destructorDefinition) {
    destructorDefinition->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  for (auto* conv : convertorDefinitions) {
    conv->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  for (auto* cons : constructorDefinitions) {
    cons->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  for (auto* memDef : memberDefinitions) {
    memDef->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  for (auto* oprDef : operatorDefinitions) {
    oprDef->setMemberParent(IR::MemberParent::CreateFromExpanded(getCoreType()));
  }
  ctx->unsetActiveType();
}

IR::CoreType* DefineCoreType::getCoreType() const { return coreTypes.back(); }

void DefineCoreType::defineType(IR::Context* ctx) {
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
  for (auto* gen : generics) {
    gen->emit(ctx);
  }
  if (!isGeneric()) {
    createType(ctx);
  } else {
    genericCoreType =
        new IR::GenericCoreType(name, generics, constraint, this, ctx->getMod(), ctx->getVisibInfo(visibSpec));
  }
}

void DefineCoreType::define(IR::Context* ctx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  if (isGeneric()) {
    createType(ctx);
    ctx->setActiveType(getCoreType());
  }
  if (defaultConstructor) {
    defaultConstructor->define(ctx);
  }
  if (copyConstructor) {
    SHOW("Defining copy constructor for " << getCoreType()->toString())
    copyConstructor->define(ctx);
  }
  if (moveConstructor) {
    SHOW("Defining move constructor for " << getCoreType()->toString())
    moveConstructor->define(ctx);
  }
  if (copyAssignment) {
    copyAssignment->define(ctx);
  }
  if (moveAssignment) {
    moveAssignment->define(ctx);
  }
  if (destructorDefinition) {
    destructorDefinition->define(ctx);
  }
  for (auto* cons : constructorDefinitions) {
    cons->define(ctx);
  }
  for (auto* conv : convertorDefinitions) {
    conv->define(ctx);
  }
  for (auto* mFn : memberDefinitions) {
    mFn->define(ctx);
  }
  for (auto* oFn : operatorDefinitions) {
    oFn->define(ctx);
  }
  if (isGeneric()) {
    ctx->unsetActiveType();
  }
}

IR::Value* DefineCoreType::emit(IR::Context* ctx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return nullptr;
  }
  ctx->setActiveType(getCoreType());
  if (defaultConstructor) {
    (void)defaultConstructor->emit(ctx);
  }
  if (copyConstructor) {
    (void)copyConstructor->emit(ctx);
  }
  if (moveConstructor) {
    (void)moveConstructor->emit(ctx);
  }
  for (auto* cons : constructorDefinitions) {
    (void)cons->emit(ctx);
  }
  for (auto* conv : convertorDefinitions) {
    (void)conv->emit(ctx);
  }
  for (auto* mFn : memberDefinitions) {
    (void)mFn->emit(ctx);
  }
  for (auto* oFn : operatorDefinitions) {
    (void)oFn->emit(ctx);
  }
  if (copyAssignment) {
    (void)copyAssignment->emit(ctx);
  }
  if (moveAssignment) {
    (void)moveAssignment->emit(ctx);
  }
  if (destructorDefinition) {
    (void)destructorDefinition->emit(ctx);
  }
  // TODO - Member function call tree analysis
  ctx->unsetActiveType();
  return nullptr;
}

void DefineCoreType::addMember(Member* mem) { members.push_back(mem); }

void DefineCoreType::addStaticMember(StaticMember* stm) { staticMembers.push_back(stm); }

void DefineCoreType::addMemberDefinition(MemberDefinition* mdef) { memberDefinitions.push_back(mdef); }

void DefineCoreType::addConvertorDefinition(ConvertorDefinition* cdef) { convertorDefinitions.push_back(cdef); }

void DefineCoreType::addConstructorDefinition(ConstructorDefinition* cdef) { constructorDefinitions.push_back(cdef); }

void DefineCoreType::addOperatorDefinition(OperatorDefinition* odef) { operatorDefinitions.push_back(odef); }

void DefineCoreType::setDestructorDefinition(DestructorDefinition* ddef) { destructorDefinition = ddef; }

void DefineCoreType::setDefaultConstructor(ConstructorDefinition* cDef) { defaultConstructor = cDef; }

bool DefineCoreType::hasDestructor() const { return destructorDefinition != nullptr; }

bool DefineCoreType::hasDefaultConstructor() const { return defaultConstructor != nullptr; }

bool DefineCoreType::hasCopyConstructor() const { return copyConstructor != nullptr; }

bool DefineCoreType::hasMoveConstructor() const { return moveConstructor != nullptr; }

bool DefineCoreType::hasCopyAssignment() const { return copyAssignment != nullptr; }

bool DefineCoreType::hasMoveAssignment() const { return moveAssignment != nullptr; }

void DefineCoreType::setCopyConstructor(ConstructorDefinition* cDef) { copyConstructor = cDef; }

void DefineCoreType::setMoveConstructor(ConstructorDefinition* cDef) { moveConstructor = cDef; }

void DefineCoreType::setCopyAssignment(OperatorDefinition* mDef) { copyAssignment = mDef; }

void DefineCoreType::setMoveAssignment(OperatorDefinition* mDef) { moveAssignment = mDef; }

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
      ._("isPacked", isPacked)
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