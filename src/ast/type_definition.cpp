#include "./type_definition.hpp"
#include "expression.hpp"

namespace qat::ast {

TypeDefinition::TypeDefinition(String _name, QatType *_subType,
                               utils::FileRange      _fileRange,
                               utils::VisibilityKind _visibKind)
    : Node(std::move(_fileRange)), name(std::move(_name)), subType(_subType),
      visibKind(_visibKind) {}

void TypeDefinition::defineType(IR::Context *ctx) {
  auto *mod     = ctx->getMod();
  auto  reqInfo = ctx->getReqInfo();
  if (mod->hasCoreType(name)) {
    ctx->Error("A core type named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtCoreType(name)) {
    ctx->Error("A core type named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasMixType(name)) {
    ctx->Error("A mix type named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtMixType(name)) {
    ctx->Error("A mix type named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasChoiceType(name)) {
    ctx->Error("A choice type named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtChoiceType(name)) {
    ctx->Error("A choice type named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleCoreTypeInImports(name, reqInfo).first) {
    ctx->Error(
        "A core type named " + ctx->highlightError(name) +
            " is present inside the module " +
            ctx->highlightError(
                mod->hasAccessibleCoreTypeInImports(name, reqInfo).second) +
            " and that module is brought into the current module.",
        fileRange);
  } else if (mod->hasTypeDef(name)) {
    ctx->Error("A type definition named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtTypeDef(name)) {
    ctx->Error("A type definition named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleTypeDefInImports(name, reqInfo).first) {
    ctx->Error(
        "A type definition named " + ctx->highlightError(name) +
            " is present inside the module " +
            ctx->highlightError(
                mod->hasAccessibleCoreTypeInImports(name, reqInfo).second) +
            " and that module is brought into the current module.",
        fileRange);
  } else if (mod->hasFunction(name)) {
    ctx->Error("A function named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtFunction(name)) {
    ctx->Error("A function named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleFunctionInImports(name, reqInfo).first) {
    ctx->Error(
        "A function named " + ctx->highlightError(name) +
            " is present inside the module " +
            ctx->highlightError(
                mod->hasAccessibleCoreTypeInImports(name, reqInfo).second) +
            ". This module is brought into the current module.",
        fileRange);
  } else if (mod->hasGlobalEntity(name)) {
    ctx->Error("A global entity named " + ctx->highlightError(name) +
                   " exists in this module. Please change name of this type "
                   "definition or check the logic",
               fileRange);
  } else if (mod->hasBroughtGlobalEntity(name)) {
    ctx->Error("A global entity named " + ctx->highlightError(name) +
                   " is brought into this module. Please change name of this "
                   "type or check the logic",
               fileRange);
  } else if (mod->hasAccessibleGlobalEntityInImports(name, reqInfo).first) {
    ctx->Error(
        "A global entity named " + ctx->highlightError(name) +
            " is present inside the module " +
            ctx->highlightError(
                mod->hasAccessibleCoreTypeInImports(name, reqInfo).second) +
            ". This module brought into the current module.",
        fileRange);
  } else {
    SHOW("Creating new type definition")
    new IR::DefinitionType(name, subType->emit(ctx), mod,
                           ctx->getVisibInfo(visibKind));
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