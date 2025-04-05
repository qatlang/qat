#include "./address_of.hpp"

namespace qat::ast {

ir::Value* AddressOf::emit(EmitCtx* ctx) {
	auto inst = instance->emit(ctx);
	if (inst->is_ref() || inst->is_ghost_ref()) {
		auto subTy    = inst->is_ref() ? inst->get_ir_type()->as_ref()->get_subtype() : inst->get_ir_type();
		bool isPtrVar = inst->is_ref() ? inst->get_ir_type()->as_ref()->has_variability() : inst->is_variable();
		if (inst->is_ref()) {
			inst->load_ghost_ref(ctx->irCtx->builder);
		}
		return ir::Value::get(inst->get_llvm(),
		                      ir::PtrType::get(isPtrVar, subTy, true, ir::PtrOwner::of_anonymous(), false, ctx->irCtx),
		                      false)
		    ->with_range(fileRange);
	} else {
		ctx->Error("The expression provided is of type " + ctx->color(inst->get_ir_type()->to_string()) +
		               ". It is not a reference, local or global value, so its address cannot be retrieved",
		           fileRange);
	}
	return nullptr;
}

Json AddressOf::to_json() const {
	return Json()._("nodeType", "addressOf")._("instance", instance->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
