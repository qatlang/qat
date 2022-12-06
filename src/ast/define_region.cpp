#include "./define_region.hpp"
#include "../IR/types/region.hpp"
#include "node.hpp"

namespace qat::ast {

DefineRegion::DefineRegion(String _name, utils::VisibilityKind _visibKind, utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), visibKind(_visibKind) {}

void DefineRegion::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  if ((!mod->hasTemplateCoreType(name)) && !mod->hasCoreType(name) && !mod->hasFunction(name) &&
      !mod->hasGlobalEntity(name) && !mod->hasBox(name) && !mod->hasTypeDef(name) && !mod->hasMixType(name) &&
      !mod->hasRegion(name)) {
    IR::Region::get(name, mod, ctx->getVisibInfo(visibKind), ctx);
  } else {
    // TODO - Check type definitions
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. Please change name of region type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing core type in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing mix type in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing choice type in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing function in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing global value in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing box in this scope. Please change name of this region or check the codebase",
          fileRange);
    } else if (mod->hasRegion(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing region in this scope. Please change name of this region or check the codebase",
          fileRange);
    }
  }
}

Json DefineRegion::toJson() const {
  return Json()
      ._("nodeType", "defineRegion")
      ._("name", name)
      ._("visibilityKind", utils::kindToJsonValue(visibKind))
      ._("fileRange", fileRange);
}

} // namespace qat::ast