#include "./define_region.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/region.hpp"
#include "./emit_ctx.hpp"
#include "./expression.hpp"
#include "./node.hpp"

namespace qat::ast {

void DefineRegion::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	mod->entity_name_check(irCtx, name, ir::EntityType::region);
	entityState = mod->add_entity(name, ir::EntityType::region, this, ir::EmitPhase::phase_1);
}

void DefineRegion::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	if (blockSize) {
		blockSize->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState,
		                               EmitCtx::get(irCtx, mod));
	}
}

void DefineRegion::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	usize blockSizeResult = 4096;
	auto  ctx             = EmitCtx::get(irCtx, mod);
	if (blockSize) {
		if (blockSize->has_type_inferrance()) {
			blockSize->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(64u, irCtx));
		}
		auto blRes = blockSize->emit(ctx);
		if ((not blRes->get_ir_type()->is_unsigned()) || (blRes->get_ir_type()->as_unsigned()->get_bitwidth() != 64u)) {
			irCtx->Error("The value provided has a type of " + irCtx->color(blRes->get_ir_type()->to_string()) +
			                 ", but the block size is expected to be of " + irCtx->color("u64") + " type",
			             blockSize->fileRange);
		}
	}
	(void)ir::Region::get(name, blockSizeResult, mod, ctx->get_visibility_info(visibSpec), irCtx, fileRange);
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
