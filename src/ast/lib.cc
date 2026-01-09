#include "./lib.hpp"
#include "emit_ctx.hpp"

namespace qat::ast {

void Lib::create_module(ir::Mod* mod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	emitCtx->name_check_in_module(name, "lib", None);
	SHOW("Creating lib")
	mod->open_lib_for_creation(name, *fileRange->file, emitCtx->get_visibility_info(visibSpec), irCtx);
	mod->get_active()->nodes = members;
	mod->close_lib_after_creation();
}

} // namespace qat::ast
