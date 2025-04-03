#include "./confirm_ref.hpp"

namespace qat::ast {

void ConfirmRef::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                     EmitCtx* ctx) {
	UPDATE_DEPS(subExpr);
}

ir::Value* ConfirmRef::emit(EmitCtx* ctx) {
	auto expr = subExpr->emit(ctx);
	if (expr->is_ref()) {
		expr->load_ghost_ref(ctx->irCtx->builder);
		if (isVar && not expr->get_ir_type()->as_ref()->has_variability()) {
			ctx->Error(
			    "This expression is a reference without variability, hence it cannot be confirmed as a reference with variability. The type of this expression is " +
			        ctx->color(expr->get_ir_type()->to_string()),
			    subExpr->fileRange);
		}
		auto res = ir::Value::get(expr->get_llvm(), expr->get_ir_type(), false);
		res->set_confirmed_ref();
		return res->with_range(fileRange);
	} else if (expr->is_ghost_ref()) {
		if (isVar && not expr->is_variable()) {
			ctx->Error(
			    "This reference-like expression does not have variability, hence it cannot be confirmed as a reference with variability",
			    subExpr->fileRange,
			    expr->has_associated_range() ? std::make_optional(std::make_pair(
			                                       "The origin of this reference-like expression can be found here",
			                                       expr->get_associated_range()))
			                                 : None);
		}
		auto res = ir::Value::get(
		    ctx->irCtx->builder.CreatePointerCast(
		        expr->get_llvm(),
		        llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace())),
		    expr->get_ir_type(), false);
		res->set_confirmed_ref();
		return res->with_range(fileRange);
	} else {
		ctx->Error(
		    "This expression is not a reference or reference-like and hence cannot be confirmed to be used as a reference. The expression is a value of type " +
		        ctx->color(expr->get_ir_type()->to_string()),
		    subExpr->fileRange);
	}
}

Json ConfirmRef::to_json() const {
	return Json()._("nodeType", "CONFIRM_REF")._("subExpression", subExpr->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
