#include "./type_definition.hpp"
#include "expression.hpp"

namespace qat::ast {

TypeDefinition::TypeDefinition(Identifier _name, QatType* _subType, FileRange _fileRange,
                               utils::VisibilityKind _visibKind)
    : Node(std::move(_fileRange)), name(std::move(_name)), subType(_subType), visibKind(_visibKind) {}

void TypeDefinition::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheck(name, "type definition", None);
  SHOW("Creating new type definition")
  new IR::DefinitionType(name, subType->emit(ctx), mod, ctx->getVisibInfo(visibKind));
  SHOW("Type definition created")
}

Json TypeDefinition::toJson() const {
  return Json()
      ._("nodeType", "typeDefinition")
      ._("name", name)
      ._("subType", subType->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast