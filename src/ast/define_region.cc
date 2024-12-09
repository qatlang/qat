#include "./define_region.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/region.hpp"
#include "emit_ctx.hpp"
#include "node.hpp"

namespace qat::ast {

void DefineRegion::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	mod->entity_name_check(irCtx, name, ir::EntityType::region);
	entityState = mod->add_entity(name, ir::EntityType::region, this, ir::EmitPhase::phase_1);
}

void DefineRegion::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	(void)ir::Region::get(name, mod, EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx, fileRange);
}

Json DefineRegion::to_json() const {
	return Json()
	    ._("nodeType", "defineRegion")
	    ._("name", name)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast