#include "./swap.hpp"
#include "../../IR/logic.hpp"

namespace qat::ast {

ir::Value* Swap::emit(EmitCtx* ctx) {
	if (isSelf) {
		if (not ctx->get_fn()->is_method()) {
			ctx->Error("Cannot perform swap on the parent instance as this is not a method", fileRange);
		} else {
			auto memFn = (ir::Method*)ctx->get_fn();
			if (memFn->is_static_method()) {
				ctx->Error("Cannot perform swap on the parent instance as this is a static function", fileRange);
			}
			if (memFn->is_constructor()) {
				ctx->Error("Cannot perform swap on the parent instance as this is a constructor", fileRange);
			}
		}
	} else {
		if (candidate->nodeType() == NodeType::SELF) {
			ctx->Error("Do not use this syntax for swapping the parent instance. Use " + ctx->color("''swap") +
			               " instead",
			           fileRange);
		}
	}
	auto* const cand = candidate->emit(ctx);
	if (not cand->is_ref() && not cand->is_ghost_ref()) {
		ctx->Error(
		    "Expected a reference or a reference-like expression with variability here, but got an expression of type " +
		        ctx->color(cand->get_ir_type()->to_string()) + " instead. This expression cannot be swapped",
		    fileRange);
	}
	if (not(cand->is_ref() ? cand->get_ir_type()->as_ref()->has_variability() : cand->has_variability())) {
		ctx->Error("This " + ctx->color(cand->is_ref() ? "reference" : "reference-like") +
		               " expression does not have variability and hence cannot be swapped",
		           fileRange);
	}
	auto candTy = cand->get_ir_type()->is_ref() ? cand->get_ir_type()->as_ref()->get_subtype() : cand->get_ir_type();
	if (candTy->is_atomic()) {
		auto val = value->emit(ctx);
		val      = ir::Logic::handle_pass_semantics(ctx, val->get_pass_type(), val, value->fileRange);
		if (not val->get_ir_type()->is_same(candTy->as_atomic()->get_subtype())) {
			ctx->Error("Expected a value of type " + ctx->color(candTy->as_atomic()->get_subtype()->to_string()) +
			               " here, but got an expression of type " + ctx->color(val->get_ir_type()->to_string()),
			           value->fileRange);
		}
		return ir::Value::get(ctx->irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg, cand->get_llvm(),
		                                                          val->get_llvm(), None,
		                                                          llvm::AtomicOrdering::SequentiallyConsistent),
		                      candTy->as_atomic()->get_subtype(), true);
	} else if (value->isInPlaceCreatable()) {
		auto ogValue = ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), cand->get_llvm());
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(candTy);
		}
		value->asInPlaceCreatable()->setCreateIn(cand);
		(void)value->emit(ctx);
		return ir::Value::get(ogValue, candTy, true);
	} else {
		auto* val = value->emit(ctx);
		val       = ir::Logic::handle_pass_semantics(ctx, val->get_pass_type(), val, value->fileRange);
		if (not val->get_ir_type()->is_same(candTy)) {
			ctx->Error("Expected a value of type " + ctx->color(candTy->to_string()) +
			               " here, but got an expression of type " + ctx->color(val->get_ir_type()->to_string()) +
			               " instead",
			           value->fileRange);
		}
		auto ogValue = ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), cand->get_llvm());
		(void)ctx->irCtx->builder.CreateStore(val->get_llvm(), cand->get_llvm());
		return ir::Value::get(ogValue, candTy, true);
	}
}

} // namespace qat::ast
