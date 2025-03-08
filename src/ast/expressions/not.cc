#include "./not.hpp"

namespace qat::ast {

ir::Value* LogicalNot::emit(EmitCtx* ctx) {
	auto* expEmit = exp->emit(ctx);
	auto* expTy   = expEmit->get_ir_type();
	if (expTy->is_ref()) {
		expTy = expTy->as_ref()->get_subtype();
	}
	if (expTy->is_bool()) {
		if (expEmit->is_ghost_ref() || expEmit->is_ref()) {
			expEmit->load_ghost_ref(ctx->irCtx->builder);
			if (expEmit->is_ref()) {
				expEmit = ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), expEmit->get_llvm()),
				                         expTy, false);
			}
		}
		return ir::Value::get(ctx->irCtx->builder.CreateNot(expEmit->get_llvm()), expTy, false);
	} else {
		ctx->Error("Invalid expression for the not operator. The expression provided is of type " +
		               ctx->color(expTy->to_string()),
		           fileRange);
	}
	return nullptr;
}

Json LogicalNot::to_json() const {
	return Json()._("nodeType", "notExpression")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
