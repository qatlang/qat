#include "./define_region.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/region.hpp"
#include "node.hpp"

namespace qat::ast {

void DefineRegion::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(ctx, name, IR::EntityType::region);
  entityState = mod->add_entity(name, IR::EntityType::region, this, IR::EmitPhase::phase_1);
}

void DefineRegion::do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) {
  (void)IR::Region::get(name, mod, ctx->getVisibInfo(visibSpec), ctx, fileRange);
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