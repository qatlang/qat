#include "./define_core_type.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/core_type.hpp"
#include "../utils/identifier.hpp"
#include "constructor.hpp"
#include "destructor.hpp"
#include "types/generic_abstract.hpp"
#include <algorithm>

namespace qat::ast {

DefineCoreType::Member::Member( //
    QatType* _type, Identifier _name, bool _variability, VisibilityKind _kind, FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability), visibilityKind(_kind),
      fileRange(std::move(_fileRange)) {}

Json DefineCoreType::Member::toJson() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", kindToJsonValue(visibilityKind))
      ._("fileRange", fileRange);
}

DefineCoreType::StaticMember::StaticMember(QatType* _type, Identifier _name, bool _variability, Expression* _value,
                                           VisibilityKind _kind, FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability), value(_value), visibilityKind(_kind),
      fileRange(std::move(_fileRange)) {}

Json DefineCoreType::StaticMember::toJson() const {
  return Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", kindToJsonValue(visibilityKind))
      ._("fileRange", fileRange);
}

DefineCoreType::DefineCoreType(Identifier _name, VisibilityKind _visibility, FileRange _fileRange,
                               Vec<ast::GenericAbstractType*> _generics, bool _isPacked)
    : Node(std::move(_fileRange)), name(std::move(_name)), isPacked(_isPacked), visibility(_visibility),
      generics(std::move(_generics)) {
  SHOW("Created define core type " + name.value)
}

bool DefineCoreType::isGeneric() const { return !(generics.empty()); }

void DefineCoreType::createType(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, isGeneric() ? "generic core type" : "core type",
                         isGeneric() ? Maybe<String>(genericCoreType->getID()) : None);
  bool                       needsDestructor = false;
  Vec<IR::CoreType::Member*> mems;
  for (auto* mem : members) {
    auto* memTy = mem->type->emit(ctx);
    if (memTy->isDestructible()) {
      needsDestructor = true;
    }
    mems.push_back(new IR::CoreType::Member(mem->name, memTy, mem->variability,
                                            (mem->visibilityKind == VisibilityKind::type)
                                                ? VisibilityInfo::type(mod->getFullNameWithChild(name.value))
                                                : ctx->getVisibInfo(mem->visibilityKind)));
  }
  auto cTyName = name;
  if (isGeneric()) {
    cTyName = Identifier(variantName.value(), name.range);
  }
  Vec<IR::GenericParameter*> genericsIR;
  for (auto* gen : generics) {
    if (!gen->isSet()) {
      if (gen->isTyped()) {
        ctx->Error("No type is set for the generic type " + ctx->highlightError(gen->getName().value) +
                       " and there is no default type provided",
                   gen->getRange());
      } else if (gen->isConst()) {
        ctx->Error("No value is set for the generic const expression " + ctx->highlightError(gen->getName().value) +
                       " and there is no default expression provided",
                   gen->getRange());
      } else {
        ctx->Error("Invalid generic kind", gen->getRange());
      }
    }
    genericsIR.push_back(gen->toIRGenericType());
  }
  coreType = new IR::CoreType(mod, cTyName, genericsIR, mems, ctx->getVisibInfo(visibility), ctx->llctx);
  if (destructorDefinition) {
    coreType->hasDefinedDestructor = true;
  }
  if (needsDestructor) {
    coreType->needsImplicitDestructor = true;
  }
  if (destructorDefinition || needsDestructor) {
    coreType->createDestructor(fileRange, ctx);
  }
  for (auto* stm : staticMembers) {
    coreType->addStaticMember(stm->name, stm->type->emit(ctx), stm->variability,
                              stm->value ? stm->value->emit(ctx) : nullptr, ctx->getVisibInfo(stm->visibilityKind),
                              ctx->llctx);
  }
  if (defaultConstructor) {
    defaultConstructor->setCoreType(coreType);
  }
  if (copyConstructor) {
    copyConstructor->setCoreType(coreType);
    if (!copyAssignment) {
      ctx->Error("Copy constructor is defined for the type " + ctx->highlightError(coreType->toString()) +
                     ", and hence copy assignment operator is also required to be defined",
                 fileRange);
    }
  }
  if (moveConstructor) {
    moveConstructor->setCoreType(coreType);
    if (!moveAssignment) {
      ctx->Error("Move constructor is defined for the type " + ctx->highlightError(coreType->toString()) +
                     ", and hence move assignment operator is also required to be defined",
                 fileRange);
    }
  }
  for (auto* conv : convertorDefinitions) {
    conv->setCoreType(coreType);
  }
  for (auto* cons : constructorDefinitions) {
    cons->setCoreType(coreType);
  }
  for (auto* memDef : memberDefinitions) {
    memDef->setCoreType(coreType);
  }
  for (auto* oprDef : operatorDefinitions) {
    oprDef->setCoreType(coreType);
  }
  if (copyAssignment) {
    copyAssignment->setCoreType(coreType);
    if (!copyConstructor) {
      ctx->Error("Copy assignment operator is defined for the type " + ctx->highlightError(coreType->toString()) +
                     ", and hence copy constructor is also required to be defined",
                 fileRange);
    }
  }
  if (moveAssignment) {
    moveAssignment->setCoreType(coreType);
    if (!moveConstructor) {
      ctx->Error("Move assignment operator is defined for the type " + ctx->highlightError(coreType->toString()) +
                     ", and hence move constructor is also required to be defined",
                 fileRange);
    }
  }
  if (destructorDefinition) {
    destructorDefinition->setCoreType(coreType);
  } else {
    destructorDefinition = new ast::DestructorDefinition(fileRange, {}, {"", {0u, 0u}, {0u, 0u}});
    destructorDefinition->setCoreType(coreType);
  }
}

IR::CoreType* DefineCoreType::getCoreType() { return coreType; }

void DefineCoreType::defineType(IR::Context* ctx) {
  for (auto* gen : generics) {
    gen->emit(ctx);
  }
  if (!isGeneric()) {
    createType(ctx);
  } else {
    genericCoreType = new IR::GenericCoreType(name, generics, this, ctx->getMod(), ctx->getVisibInfo(visibility));
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
  auto prevActiveType = ctx->activeType;
  ctx->activeType     = coreType;
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
  ctx->activeType = prevActiveType;
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
      ._("visibility", kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

DefineCoreType::~DefineCoreType() {
  for (auto* gen : generics) {
    delete gen;
  }
}

} // namespace qat::ast