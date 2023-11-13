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

DefineCoreType::Member::Member( //
    QatType* _type, Identifier _name, bool _variability, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability), visibSpec(_visibSpec),
      fileRange(std::move(_fileRange)) {}

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

DefineCoreType::StaticMember::StaticMember(QatType* _type, Identifier _name, bool _variability, Expression* _value,
                                           Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability), value(_value), visibSpec(_visibSpec),
      fileRange(std::move(_fileRange)) {}

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

DefineCoreType::DefineCoreType(Identifier _name, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange,
                               Vec<ast::GenericAbstractType*> _generics, bool _isPacked)
    : Node(std::move(_fileRange)), name(std::move(_name)), isPacked(_isPacked), visibSpec(_visibSpec),
      generics(std::move(_generics)) {
  SHOW("Created define core type " + name.value)
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

void DefineCoreType::createType(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, isGeneric() ? "generic core type" : "core type",
                         isGeneric() ? Maybe<String>(genericCoreType->getID()) : None);
  bool needsDestructor = false;
  auto cTyName         = name;
  if (isGeneric()) {
    cTyName = Identifier(variantName.value(), name.range);
  }
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
  setOpaque(IR::OpaqueType::get(cTyName, genericsIR, isGeneric() ? Maybe<String>(genericCoreType->getID()) : None,
                                IR::OpaqueSubtypeKind::core, mod,
                                hasAllMems ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(eqStructTy)) : None,
                                ctx->getVisibInfo(visibSpec), ctx->llctx));
  if (genericCoreType) {
    genericCoreType->opaqueVariants.push_back(
        IR::GenericVariant<IR::OpaqueType>(getOpaque(), std::move(genericsToFill)));
  }
  SHOW("Created opaque for core type")
  ctx->setActiveType(getOpaque());
  Vec<IR::CoreType::Member*> mems;
  SHOW("Generating core type members")
  for (auto* mem : members) {
    auto* memTy = mem->type->emit(ctx);
    if (memTy->isOpaque() && !memTy->asOpaque()->hasSubType()) {
      // NOTE - Support sized opaques?
      if (hasOpaque() && memTy->isSame(getOpaque())) {
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
    mems.push_back(new IR::CoreType::Member(mem->name, memTy, mem->variability,
                                            (mem->visibSpec.has_value() && mem->visibSpec->kind == VisibilityKind::type)
                                                ? VisibilityInfo::type(getOpaque())
                                                : ctx->getVisibInfo(mem->visibSpec)));
  }
  SHOW("Creating core type: " << cTyName.value)
  setCoreType(new IR::CoreType(mod, cTyName, genericsIR, getOpaque(), mems, ctx->getVisibInfo(visibSpec), ctx->llctx));
  SHOW("CoreType ID: " << coreTypes.back()->getID())
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
  if (destructorDefinition || needsDestructor) {
    getCoreType()->createDestructor(fileRange, ctx);
  }
  for (auto* stm : staticMembers) {
    getCoreType()->addStaticMember(stm->name, stm->type->emit(ctx), stm->variability,
                                   stm->value ? stm->value->emit(ctx) : nullptr, ctx->getVisibInfo(stm->visibSpec),
                                   ctx->llctx);
  }
  if (defaultConstructor) {
    defaultConstructor->setCoreType(getCoreType());
  }
  if (copyConstructor) {
    copyConstructor->setCoreType(getCoreType());
    if (!copyAssignment) {
      ctx->Error("Copy constructor is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence copy assignment operator is also required to be defined",
                 fileRange);
    }
  }
  if (moveConstructor) {
    moveConstructor->setCoreType(getCoreType());
    if (!moveAssignment) {
      ctx->Error("Move constructor is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence move assignment operator is also required to be defined",
                 fileRange);
    }
  }
  for (auto* conv : convertorDefinitions) {
    conv->setCoreType(getCoreType());
  }
  for (auto* cons : constructorDefinitions) {
    cons->setCoreType(getCoreType());
  }
  for (auto* memDef : memberDefinitions) {
    memDef->setCoreType(getCoreType());
  }
  for (auto* oprDef : operatorDefinitions) {
    oprDef->setCoreType(getCoreType());
  }
  if (copyAssignment) {
    copyAssignment->setCoreType(getCoreType());
    if (!copyConstructor) {
      ctx->Error("Copy assignment operator is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence copy constructor is also required to be defined",
                 fileRange);
    }
  }
  if (moveAssignment) {
    moveAssignment->setCoreType(getCoreType());
    if (!moveConstructor) {
      ctx->Error("Move assignment operator is defined for the type " + ctx->highlightError(getCoreType()->toString()) +
                     ", and hence move constructor is also required to be defined",
                 fileRange);
    }
  }
  if (destructorDefinition) {
    destructorDefinition->setCoreType(getCoreType());
  } else {
    destructorDefinition = new ast::DestructorDefinition(fileRange, {}, {"", {0u, 0u}, {0u, 0u}});
    destructorDefinition->setCoreType(getCoreType());
  }
  ctx->unsetActiveType();
}

IR::CoreType* DefineCoreType::getCoreType() const { return coreTypes.back(); }

void DefineCoreType::defineType(IR::Context* ctx) {
  for (auto* gen : generics) {
    gen->emit(ctx);
  }
  if (!isGeneric()) {
    createType(ctx);
  } else {
    genericCoreType = new IR::GenericCoreType(name, generics, this, ctx->getMod(), ctx->getVisibInfo(visibSpec));
  }
}

void DefineCoreType::setVariantName(const String& name) const { variantName = name; }

void DefineCoreType::unsetVariantName() const { variantName = None; }

void DefineCoreType::define(IR::Context* ctx) {
  if (isGeneric()) {
    createType(ctx);
  }
  if (defaultConstructor) {
    defaultConstructor->define(ctx);
  }
  if (copyConstructor) {
    copyConstructor->define(ctx);
  }
  if (moveConstructor) {
    moveConstructor->define(ctx);
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
  if (copyAssignment) {
    copyAssignment->define(ctx);
  }
  if (moveAssignment) {
    moveAssignment->define(ctx);
  }
  if (destructorDefinition) {
    destructorDefinition->define(ctx);
  }
}

IR::Value* DefineCoreType::emit(IR::Context* ctx) {
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
  unsetCoreType();
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
  for (auto* gen : generics) {
    delete gen;
  }
}

} // namespace qat::ast