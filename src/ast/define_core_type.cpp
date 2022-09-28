#include "./define_core_type.hpp"
#include "../IR/types/core_type.hpp"
#include "constructor.hpp"
#include "destructor.hpp"
#include "types/templated.hpp"

namespace qat::ast {

DefineCoreType::Member::Member( //
    QatType *_type, String _name, bool _variability,
    utils::VisibilityKind _kind, utils::FileRange _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability),
      visibilityKind(_kind), fileRange(std::move(_fileRange)) {}

nuo::Json DefineCoreType::Member::toJson() const {
  return nuo::Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", utils::kindToJsonValue(visibilityKind))
      ._("fileRange", fileRange);
}

DefineCoreType::StaticMember::StaticMember(QatType *_type, String _name,
                                           bool                  _variability,
                                           Expression           *_value,
                                           utils::VisibilityKind _kind,
                                           utils::FileRange      _fileRange)
    : type(_type), name(std::move(_name)), variability(_variability),
      value(_value), visibilityKind(_kind), fileRange(std::move(_fileRange)) {}

nuo::Json DefineCoreType::StaticMember::toJson() const {
  return nuo::Json()
      ._("nodeType", "coreTypeMember")
      ._("name", name)
      ._("type", type->toJson())
      ._("variability", variability)
      ._("visibility", utils::kindToJsonValue(visibilityKind))
      ._("fileRange", fileRange);
}

DefineCoreType::DefineCoreType(String                      _name,
                               const utils::VisibilityKind _visibility,
                               utils::FileRange            _fileRange,
                               Vec<ast::TemplatedType *>   _templates,
                               bool                        _isPacked)
    : Node(std::move(_fileRange)), name(std::move(_name)), isPacked(_isPacked),
      visibility(_visibility), templates(std::move(_templates)) {
  SHOW("Created core type " + name)
}

bool DefineCoreType::isTemplate() const { return !(templates.empty()); }

void DefineCoreType::createType(IR::Context *ctx) const {
  auto *mod = ctx->getMod();
  if ((isTemplate() || !mod->hasTemplateCoreType(name)) &&
      !mod->hasCoreType(name) && !mod->hasFunction(name) &&
      !mod->hasGlobalEntity(name) && !mod->hasBox(name) &&
      !mod->hasTypeDef(name) && !mod->hasMixType(name) &&
      !mod->hasChoiceType(name)) {
    SHOW("Creating IR for CoreType members. Count: "
         << std::to_string(members.size()))
    Vec<IR::CoreType::Member *> mems;
    for (auto *mem : members) {
      mems.push_back(new IR::CoreType::Member(
          mem->name, mem->type->emit(ctx), mem->variability,
          (mem->visibilityKind == utils::VisibilityKind::type)
              ? utils::VisibilityInfo::type(mod->getFullNameWithChild(name))
              : ctx->getVisibInfo(mem->visibilityKind)));
    }
    SHOW("Emitted IR for all members")
    auto cTyName = name;
    if (isTemplate()) {
      cTyName = name + "'<" + variantName.value() + ">";
    }
    coreType = new IR::CoreType(mod, cTyName, mems,
                                ctx->getVisibInfo(visibility), ctx->llctx);
    SHOW("Created core type in IR")
    for (auto *stm : staticMembers) {
      coreType->addStaticMember(
          stm->name, stm->type->emit(ctx), stm->variability,
          stm->value ? stm->value->emit(ctx) : nullptr,
          ctx->getVisibInfo(stm->visibilityKind), ctx->llctx);
    }
    if (defaultConstructor) {
      defaultConstructor->setCoreType(coreType);
    }
    if (copyConstructor) {
      copyConstructor->setCoreType(coreType);
    }
    if (moveConstructor) {
      moveConstructor->setCoreType(coreType);
    }
    for (auto *conv : convertorDefinitions) {
      conv->setCoreType(coreType);
    }
    for (auto *cons : constructorDefinitions) {
      cons->setCoreType(coreType);
    }
    for (auto *memDef : memberDefinitions) {
      memDef->setCoreType(coreType);
    }
    for (auto *oprDef : operatorDefinitions) {
      oprDef->setCoreType(coreType);
    }
    if (destructorDefinition) {
      destructorDefinition->setCoreType(coreType);
    } else {
      destructorDefinition =
          new ast::DestructorDefinition({}, {"", {0u, 0u}, {0u, 0u}});
      destructorDefinition->setCoreType(coreType);
    }
  } else {
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. "
              "Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing core type in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing mix type in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing choice type in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing function in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing global value in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing box in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    }
  }
}

IR::CoreType *DefineCoreType::getCoreType() { return coreType; }

void DefineCoreType::defineType(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if ((isTemplate() || !mod->hasTemplateCoreType(name)) &&
      !mod->hasCoreType(name) && !mod->hasFunction(name) &&
      !mod->hasGlobalEntity(name) && !mod->hasBox(name) &&
      !mod->hasTypeDef(name) && !mod->hasMixType(name) &&
      !mod->hasChoiceType(name)) {
    if (!isTemplate()) {
      createType(ctx);
    } else {
      SHOW("Creating template core type: " << name)
      templateCoreType = new IR::TemplateCoreType(
          name, templates, this, ctx->getMod(), ctx->getVisibInfo(visibility));
      SHOW("Created templated core type")
    }
  } else {
    // TODO - Check type definitions
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. "
              "Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing core type in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing mix type in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing choice type in this scope. "
              "Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing function in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing global value in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(ctx->highlightError(name) +
                     " is the name of an existing box in this scope. "
                     "Please change name of this type or check the codebase",
                 fileRange);
    }
  }
}

void DefineCoreType::setVariantName(const String &name) const {
  variantName = name;
}

void DefineCoreType::unsetVariantName() const { variantName = None; }

void DefineCoreType::define(IR::Context *ctx) {
  if (isTemplate()) {
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
  for (auto *cons : constructorDefinitions) {
    cons->define(ctx);
  }
  for (auto *conv : convertorDefinitions) {
    conv->define(ctx);
  }
  for (auto *mFn : memberDefinitions) {
    mFn->define(ctx);
  }
  for (auto *oFn : operatorDefinitions) {
    oFn->define(ctx);
  }
  if (destructorDefinition) {
    destructorDefinition->define(ctx);
  }
}

IR::Value *DefineCoreType::emit(IR::Context *ctx) {
  ctx->activeType = coreType;
  if (defaultConstructor) {
    (void)defaultConstructor->emit(ctx);
  }
  if (copyConstructor) {
    (void)copyConstructor->emit(ctx);
  }
  if (moveConstructor) {
    (void)moveConstructor->emit(ctx);
  }
  for (auto *cons : constructorDefinitions) {
    (void)cons->emit(ctx);
  }
  for (auto *conv : convertorDefinitions) {
    (void)conv->emit(ctx);
  }
  for (auto *mFn : memberDefinitions) {
    (void)mFn->emit(ctx);
  }
  for (auto *oFn : operatorDefinitions) {
    (void)oFn->emit(ctx);
  }
  if (destructorDefinition) {
    (void)destructorDefinition->emit(ctx);
  }
  ctx->activeType = nullptr;
  return nullptr;
}

void DefineCoreType::addMember(Member *mem) {
  SHOW("Added core type member: " + mem->name)
  members.push_back(mem);
}

void DefineCoreType::addStaticMember(StaticMember *stm) {
  SHOW("Added core type static member: " + stm->name)
  staticMembers.push_back(stm);
}

void DefineCoreType::addMemberDefinition(MemberDefinition *mdef) {
  memberDefinitions.push_back(mdef);
}

void DefineCoreType::addConvertorDefinition(ConvertorDefinition *cdef) {
  convertorDefinitions.push_back(cdef);
}

void DefineCoreType::addConstructorDefinition(ConstructorDefinition *cdef) {
  constructorDefinitions.push_back(cdef);
}

void DefineCoreType::addOperatorDefinition(OperatorDefinition *odef) {
  operatorDefinitions.push_back(odef);
}

void DefineCoreType::setDestructorDefinition(DestructorDefinition *ddef) {
  destructorDefinition = ddef;
}

void DefineCoreType::setDefaultConstructor(ConstructorDefinition *cDef) {
  defaultConstructor = cDef;
}

bool DefineCoreType::hasDestructor() const {
  return destructorDefinition != nullptr;
}

bool DefineCoreType::hasDefaultConstructor() const {
  return defaultConstructor != nullptr;
}

bool DefineCoreType::hasCopyConstructor() const {
  return copyConstructor != nullptr;
}

bool DefineCoreType::hasMoveConstructor() const {
  return moveConstructor != nullptr;
}

void DefineCoreType::setCopyConstructor(ConstructorDefinition *cDef) {
  copyConstructor = cDef;
}

void DefineCoreType::setMoveConstructor(ConstructorDefinition *cDef) {
  moveConstructor = cDef;
}

nuo::Json DefineCoreType::toJson() const {
  Vec<nuo::JsonValue> membersJsonValue;
  Vec<nuo::JsonValue> staticMembersJsonValue;
  for (auto *mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  for (auto *mem : staticMembers) {
    staticMembersJsonValue.emplace_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "defineCoreType")
      ._("members", membersJsonValue)
      ._("staticMembers", staticMembersJsonValue)
      ._("isPacked", isPacked)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast