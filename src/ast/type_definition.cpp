#include "./type_definition.hpp"
#include "./types/generic_abstract.hpp"
#include "expression.hpp"

namespace qat::ast {

TypeDefinition::TypeDefinition(Identifier _name, Vec<ast::GenericAbstractType*> _generics, QatType* _subType,
                               FileRange _fileRange, VisibilityKind _visibKind)
    : Node(std::move(_fileRange)), name(std::move(_name)), subType(_subType), visibKind(_visibKind),
      generics(_generics) {}

bool TypeDefinition::isGeneric() const { return !generics.empty(); }

void TypeDefinition::setVariantName(const String& name) const { variantName = name; }

void TypeDefinition::unsetVariantName() const { variantName = None; }

void TypeDefinition::createType(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, isGeneric() ? "generic type definition" : "type definition",
                         isGeneric() ? Maybe<String>(genericTypeDefinition->getID()) : None);
  auto dTyName = name;
  if (isGeneric()) {
    dTyName = Identifier(variantName.value(), name.range);
  }
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
  if (isGeneric()) {
    ctx->getActiveGeneric().generics = genericsIR;
  }
  typeDefinition = new IR::DefinitionType(dTyName, subType->emit(ctx), genericsIR, mod, ctx->getVisibInfo(visibKind));
}

void TypeDefinition::defineType(IR::Context* ctx) {
  if (isGeneric()) {
    genericTypeDefinition =
        new IR::GenericDefinitionType(name, generics, this, ctx->getMod(), ctx->getVisibInfo(visibKind));
  } else {
    createType(ctx);
  }
}

void TypeDefinition::define(IR::Context* ctx) {
  if (isGeneric()) {
    createType(ctx);
  }
}

IR::DefinitionType* TypeDefinition::getDefinition() const { return typeDefinition; }

Json TypeDefinition::toJson() const {
  Vec<JsonValue> genJson;
  for (auto* gen : generics) {
    genJson.push_back(gen->toJson());
  }
  return Json()
      ._("nodeType", "typeDefinition")
      ._("name", name)
      ._("subType", subType->toJson())
      ._("hasGenerics", !generics.empty())
      ._("generics", genJson)
      ._("visibility", kindToJsonValue(visibKind))
      ._("fileRange", fileRange);
}

} // namespace qat::ast