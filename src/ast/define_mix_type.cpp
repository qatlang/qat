#include "./define_mix_type.hpp"

namespace qat::ast {

DefineMixType::DefineMixType(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes, Vec<FileRange> _ranges,
                             Maybe<usize> _defaultVal, bool _isPacked, utils::VisibilityKind _visibility,
                             FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), subtypes(std::move(_subTypes)), isPacked(_isPacked),
      visibility(_visibility), fRanges(std::move(_ranges)), defaultVal(_defaultVal) {}

bool DefineMixType::isTemplate() const { return !templates.empty(); }

void DefineMixType::createType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  if
      //   ((isTemplate() || !mod->hasTemplateMixType(name)) &&
      (!mod->hasCoreType(name.value) && !mod->hasFunction(name.value) && !mod->hasGlobalEntity(name.value) &&
       !mod->hasBox(name.value) && !mod->hasTypeDef(name.value) && !mod->hasMixType(name.value) &&
       !mod->hasChoiceType(name.value) && !mod->hasRegion(name.value)) {
    Vec<Pair<Identifier, Maybe<IR::QatType*>>> subTypesIR;
    bool                                       hasAssociatedType = false;
    for (usize i = 0; i < subtypes.size(); i++) {
      for (usize j = i + 1; j < subtypes.size(); j++) {
        if (subtypes.at(i).first.value == subtypes.at(j).first.value) {
          ctx->Error("The name of the subtype of the union is repeating here. Please "
                     "check logic & make necessary changes",
                     fRanges.at(j));
        }
      }
      if (!hasAssociatedType && subtypes.at(i).second.has_value()) {
        hasAssociatedType = true;
      }
      if (defaultVal.has_value()) {
        if (defaultVal.value() == i) {
          if (subtypes.at(i).second.has_value()) {
            ctx->Error("A subfield with a value associated with them cannot be "
                       "used as a default value",
                       {fRanges.at(i), subtypes.at(i).second.value()->fileRange});
          }
        }
      }
      subTypesIR.push_back(Pair<Identifier, Maybe<IR::QatType*>>(
          subtypes.at(i).first,
          subtypes.at(i).second.has_value() ? Maybe<IR::QatType*>(subtypes.at(i).second.value()->emit(ctx)) : None));
    }
    if (!hasAssociatedType) {
      ctx->Error("No types associated to any of the subfields of the mix type. "
                 "Please change this type to a choice type",
                 fileRange);
    }
    new IR::MixType(name, mod, subTypesIR, defaultVal, ctx->llctx, isPacked, ctx->getVisibInfo(visibility), fileRange);
  } else {
    if (mod->hasTemplateCoreType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing template core type in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing core type in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing type definition in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing mix type in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing choice type in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing function in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing global value in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasBox(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing box in this scope. Please change name of this union type or check the codebase",
          fileRange);
    } else if (mod->hasRegion(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing region in this scope. Please change name of this region or check the codebase",
          fileRange);
    }
  }
}

void DefineMixType::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  if ((isTemplate() || !mod->hasTemplateCoreType(name.value)) && !mod->hasCoreType(name.value) &&
      !mod->hasFunction(name.value) && !mod->hasGlobalEntity(name.value) && !mod->hasBox(name.value) &&
      !mod->hasTypeDef(name.value) && !mod->hasMixType(name.value) && !mod->hasRegion(name.value)) {
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
    if (mod->hasTemplateCoreType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing template core type in this scope. Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing core type in this scope. Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing type definition in this scope. Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing mix type in this scope. Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing choice type in this scope. Please change name of this core type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing function in this scope. Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing global value in this scope. Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasBox(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing box in this scope. Please change name of this type or check the codebase",
          fileRange);
    } else if (mod->hasRegion(name.value)) {
      ctx->Error(
          ctx->highlightError(name.value) +
              " is the name of an existing region in this scope. Please change name of this region or check the codebase",
          fileRange);
    }
  }
}

IR::Value* DefineMixType::emit(IR::Context* ctx) { return nullptr; }

Json DefineMixType::toJson() const {
  Vec<JsonValue> subTypesJson;
  for (const auto& sub : subtypes) {
    subTypesJson.push_back(Json()
                               ._("name", sub.first)
                               ._("hasType", sub.second.has_value())
                               ._("type", sub.second.has_value() ? sub.second.value()->toJson() : JsonValue()));
  }
  return Json()._("nodeType", "defineUnionType")._("name", name)._("subTypes", subTypesJson)._("fileRange", fileRange);
}

} // namespace qat::ast