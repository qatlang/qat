#include "./inline_let.hpp"
#include "../../IR/logic.hpp"

namespace qat::ast {

void InlineLet::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                    EmitCtx* ctx) {
	UPDATE_DEPS(expression);
}

ir::Value* InlineLet::emit(EmitCtx* ctx) {
	auto val = expression->emit(ctx);
	if (val->is_local_value()) {
		ctx->Error("This expression is already a local value, so there is no need to allocate it in-place using " +
		               ctx->color("'let"),
		           expression->fileRange);
	}
	val      = ir::Logic::handle_pass_semantics(ctx, val->get_pass_type(), val, expression->fileRange);
	auto res = ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), val->get_ir_type(), true,
	                                                 fileRange);
	ctx->irCtx->builder.CreateStore(val->get_llvm(), res->get_llvm());
	return res;
}

Json InlineLet::to_json() const {
	return Json()._("nodeType", "inlineLet")._("expression", expression->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
