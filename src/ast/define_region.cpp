#include "./define_region.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/region.hpp"
#include "node.hpp"

namespace qat::ast {

void DefineRegion::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "region", None);
  IR::Region::get(name, mod, ctx->getVisibInfo(visibSpec), ctx, fileRange);
}

Json DefineRegion::toJson() const {
  return Json()
      ._("nodeType", "defineRegion")
      ._("name", name)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast