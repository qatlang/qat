#include "./type_definition.hpp"
#include "expression.hpp"

namespace qat::ast {

TypeDefinition::TypeDefinition(Identifier _name, QatType* _subType, FileRange _fileRange,
                               utils::VisibilityKind _visibKind)
    : Node(std::move(_fileRange)), name(std::move(_name)), subType(_subType), visibKind(_visibKind) {}

void TypeDefinition::defineType(IR::Context* ctx) {
  auto* mod     = ctx->getMod();
  auto  reqInfo = ctx->getReqInfo();
  if (mod->hasCoreType(name.value)) {
    ctx->Error("A core type named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtCoreType(name.value)) {
    ctx->Error("A core type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasMixType(name.value)) {
    ctx->Error("A mix type named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtMixType(name.value)) {
    ctx->Error("A mix type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasChoiceType(name.value)) {
    ctx->Error("A choice type named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtChoiceType(name.value)) {
    ctx->Error("A choice type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).first) {
    ctx->Error("A core type named " + ctx->highlightError(name.value) + " is present inside the module " +
                   ctx->highlightError(mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
                   " and that module is brought into the current module.",
               fileRange);
  } else if (mod->hasTypeDef(name.value)) {
    ctx->Error("A type definition named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtTypeDef(name.value)) {
    ctx->Error("A type definition named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleTypeDefInImports(name.value, reqInfo).first) {
    ctx->Error("A type definition named " + ctx->highlightError(name.value) + " is present inside the module " +
                   ctx->highlightError(mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
                   " and that module is brought into the current module.",
               fileRange);
  } else if (mod->hasFunction(name.value)) {
    ctx->Error("A function named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtFunction(name.value)) {
    ctx->Error("A function named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleFunctionInImports(name.value, reqInfo).first) {
    ctx->Error("A function named " + ctx->highlightError(name.value) + " is present inside the module " +
                   ctx->highlightError(mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
                   ". This module is brought into the current module.",
               fileRange);
  } else if (mod->hasGlobalEntity(name.value)) {
    ctx->Error("A global entity named " + ctx->highlightError(name.value) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtGlobalEntity(name.value)) {
    ctx->Error("A global entity named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleGlobalEntityInImports(name.value, reqInfo).first) {
    ctx->Error("A global entity named " + ctx->highlightError(name.value) + " is present inside the module " +
                   ctx->highlightError(mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
                   ". This module brought into the current module.",
               fileRange);
  } else {
    SHOW("Creating new type definition")
    new IR::DefinitionType(name, subType->emit(ctx), mod, ctx->getVisibInfo(visibKind));
    SHOW("Type definition created")
  }
}

Json TypeDefinition::toJson() const {
  return Json()
      ._("nodeType", "typeDefinition")
      ._("name", name)
      ._("subType", subType->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast