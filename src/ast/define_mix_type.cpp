#include "./define_mix_type.hpp"

namespace qat::ast {

DefineMixType::DefineMixType(String                              _name,
                             Vec<Pair<String, Maybe<QatType *>>> _subTypes,
                             Vec<utils::FileRange> _ranges, bool _isPacked,
                             utils::VisibilityKind _visibility,
                             utils::FileRange      _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)),
      subtypes(std::move(_subTypes)), isPacked(_isPacked),
      visibility(_visibility), fRanges(std::move(_ranges)) {}

bool DefineMixType::isTemplate() const { return !templates.empty(); }

void DefineMixType::createType(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if
      //   ((isTemplate() || !mod->hasTemplateMixType(name)) &&
      (!mod->hasCoreType(name) && !mod->hasFunction(name) &&
       !mod->hasGlobalEntity(name) && !mod->hasBox(name) &&
       !mod->hasTypeDef(name) && !mod->hasMixType(name)) {
    Vec<Pair<String, Maybe<IR::QatType *>>> subTypesIR;
    bool                                    hasAssociatedType = false;
    for (usize i = 0; i < subtypes.size(); i++) {
      for (usize j = i + 1; j < subtypes.size(); j++) {
        if (subtypes.at(i).first == subtypes.at(j).first) {
          ctx->Error(
              "The name of the subtype of the union is repeating here. Please "
              "check logic & make necessary changes",
              fRanges.at(j));
        }
      }
      if (!hasAssociatedType && subtypes.at(i).second.has_value()) {
        hasAssociatedType = true;
      }
      subTypesIR.push_back(Pair<String, Maybe<IR::QatType *>>(
          subtypes.at(i).first,
          subtypes.at(i).second.has_value()
              ? Maybe<IR::QatType *>(subtypes.at(i).second.value()->emit(ctx))
              : None));
    }
    if (!hasAssociatedType) {
      ctx->Error("No types associated to any of the subfields of the mix type. "
                 "Please change this type to a choice type",
                 fileRange);
    }
    new IR::MixType(name, mod, subTypesIR, ctx->llctx, isPacked,
                    ctx->getVisibInfo(visibility));
  } else {
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing core type in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing union type in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing function in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing global value in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing box in this scope. "
              "Please change name of this union type or check the codebase",
          fileRange);
    }
  }
}

void DefineMixType::defineType(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if ((isTemplate() || !mod->hasTemplateCoreType(name)) &&
      !mod->hasCoreType(name) && !mod->hasFunction(name) &&
      !mod->hasGlobalEntity(name) && !mod->hasBox(name) &&
      !mod->hasTypeDef(name) && !mod->hasMixType(name)) {
    if (!isTemplate()) {
      createType(ctx);
    } else {
      //   SHOW("Creating template union type: " << name)
      //   templateUnionType = new IR::TemplateUnionType(
      //       name, templates, this, ctx->getMod(),
      //       ctx->getVisibInfo(visibility));
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
              " is the name of an existing union type in this scope. "
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

IR::Value *DefineMixType::emit(IR::Context *ctx) { return nullptr; }

nuo::Json DefineMixType::toJson() const {
  Vec<nuo::JsonValue> subTypesJson;
  for (const auto &sub : subtypes) {
    subTypesJson.push_back(nuo::Json()
                               ._("name", sub.first)
                               ._("hasType", sub.second.has_value())
                               ._("type", sub.second.has_value()
                                              ? sub.second.value()->toJson()
                                              : nuo::JsonValue()));
  }
  return nuo::Json()
      ._("nodeType", "defineUnionType")
      ._("name", name)
      ._("subTypes", subTypesJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast