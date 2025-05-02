#include "./assembly_block.hpp"
#include "../IR/qat_module.hpp"
#include "./emit_ctx.hpp"
#include "./expression.hpp"

namespace qat::ast {

void AssemblyBlock::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx    = EmitCtx::get(irCtx, mod);
	entityState = mod->add_entity(None, ir::EntityType::assemblyBlock, this, ir::EmitPhase::phase_1);
}

void AssemblyBlock::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (defineChecker) {
		defineChecker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	content->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
}

void AssemblyBlock::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (defineChecker) {
		auto defCond = defineChecker->emit(ctx);
		if (not defCond->get_ir_type()->is_bool()) {
			ctx->Error("The define condition is always required to be of " + ctx->color("bool") +
			               " type. Please check the logic",
			           defineChecker->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(defCond->get_llvm_constant())->getValue().getBoolValue()) {
			return;
		}
	}
	auto cont = content->emit(ctx);
	if (not cont->get_ir_type()->is_text()) {
		ctx->Error("The body of the assembly block is required to be of " + ctx->color("text") +
		               " type. Please check the logic",
		           content->fileRange);
	}
	mod->get_llvm_module()->appendModuleInlineAsm(ir::TextType::value_to_string(cont));
}

useit Json AssemblyBlock::to_json() const {
	return Json()
	    ._("nodeType", "assemblyBlock")
	    ._("content", content->to_json())
	    ._("defineChecker", defineChecker->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
