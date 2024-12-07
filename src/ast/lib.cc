#include "./lib.hpp"
#include "emit_ctx.hpp"

namespace qat::ast {

void Lib::create_module(ir::Mod* mod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	emitCtx->name_check_in_module(name, "lib", None);
	SHOW("Creating lib")
	mod->open_lib_for_creation(name, fileRange.file.string(), emitCtx->get_visibility_info(visibSpec), irCtx);
	mod->get_active()->nodes = members;
	mod->close_lib_after_creation();
}

Json Lib::to_json() const {
	Vec<JsonValue> membersJsonValue;
	for (auto* mem : members) {
		membersJsonValue.emplace_back(mem->to_json());
	}
	return Json()
		._("name", name)
		._("nodeType", "lib")
		._("members", membersJsonValue)
		._("hasVisibility", visibSpec.has_value())
		._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
		._("fileRange", fileRange);
}

} // namespace qat::ast