#include "./mod_info.hpp"
#include <filesystem>

namespace qat::ast {

void ModInfo::create_module(ir::Mod* mod, ir::Ctx* irCtx) const {
	if (mod->metaInfo.has_value()) {
		irCtx->Error("Meta information is already provided for module " + irCtx->color(mod->name.value) + " in file " +
						 irCtx->color(mod->filePath.string()),
					 fileRange);
	}
	auto irMeta = metaInfo.toIR(EmitCtx::get(irCtx, mod));
	if (irMeta.has_key("foreign") && mod->has_meta_info_key_in_parent("foreign")) {
		irCtx->Error("The foreign ID has already been provided in one of the parent modules of this module", fileRange);
	}
	SHOW("Setting module info for " << mod->name.value << " at " << mod->filePath)
	mod->metaInfo = irMeta;
}

Json ModInfo::to_json() const { return Json()._("fileRange", fileRange); }

} // namespace qat::ast
