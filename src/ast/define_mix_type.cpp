#include "./define_mix_type.hpp"
#include "../IR/logic.hpp"

namespace qat::ast {

DefineMixType::DefineMixType(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes, Vec<FileRange> _ranges,
                             Maybe<usize> _defaultVal, bool _isPacked, utils::VisibilityKind _visibility,
                             FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), subtypes(std::move(_subTypes)), isPacked(_isPacked),
      visibility(_visibility), fRanges(std::move(_ranges)), defaultVal(_defaultVal) {}

bool DefineMixType::isTemplate() const { return !templates.empty(); }

void DefineMixType::createType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheck(name, "mix type", None);
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
  new IR::MixType(name, mod, subTypesIR, defaultVal, ctx, isPacked, ctx->getVisibInfo(visibility), fileRange);
}

void DefineMixType::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  if (!isTemplate()) {
    createType(ctx);
  } else {
    //   SHOW("Creating template mix type: " << name)
    //   templateUnionType = new IR::TemplateUnionType(
    //       name, templates, this, ctx->getMod(),
    //       ctx->getVisibInfo(visibility));
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