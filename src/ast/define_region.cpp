#include "./define_region.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/region.hpp"
#include "node.hpp"

namespace qat::ast {

DefineRegion::DefineRegion(Identifier _name, utils::VisibilityKind _visibKind, FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), visibKind(_visibKind) {}

void DefineRegion::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "region", None);
  IR::Region::get(name, mod, ctx->getVisibInfo(visibKind), ctx, fileRange);
}

Json DefineRegion::toJson() const {
  return Json()
      ._("nodeType", "defineRegion")
      ._("name", name)
      ._("visibilityKind", utils::kindToJsonValue(visibKind))
      ._("fileRange", fileRange);
}

} // namespace qat::ast