#include "./atomic_copy.hpp"
#include "../../IR/logic.hpp"
#include "./atomic_operations.hpp"

namespace qat::ast {

ir::Value* AtomicCopy::emit(EmitCtx* ctx) {
	auto cand   = candidate->emit(ctx);
	auto candTy = cand->get_ir_type();
	if (candTy->is_ref()) {
		cand->load_ghost_ref(ctx->irCtx->builder);
		candTy = candTy->as_ref()->get_subtype();
	} else if (not cand->is_ghost_ref()) {
		ctx->Error("Expected a reference or a reference-like expression here, got a value of type " +
		               ctx->color(candTy->to_string()),
		           fileRange);
	}
	if (not candTy->has_simple_copy()) {
		ctx->Error("The type " + ctx->color(candTy->to_string()) +
		               " does not have simple-copy and hence does not support atomic copy",
		           fileRange);
	}
	auto atomicCheck = ir::Logic::is_atomic_qualified_type(candTy->get_llvm_type(), ctx->irCtx);
	if (atomicCheck.has_value()) {
		if (not atomicCheck.value()) {
			ctx->Error("The size of the type " + ctx->color(candTy->to_string()) + " is " +
			               std::to_string(ctx->irCtx->dataLayout.getTypeStoreSizeInBits(candTy->get_llvm_type())) +
			               ", which exceeds the maximum limit allowed for atomic operations in the target architecture",
			           fileRange);
		}
	} else {
		ctx->irCtx->Warning("Could not determine whether the target architecture supports atomic operations for type " +
		                        ctx->color(candTy->to_string()) +
		                        ", so there is no guarantee that this operation will be atomic",
		                    fileRange);
	}
	auto load  = ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), cand->get_llvm());
	auto order = llvm::AtomicOrdering::SequentiallyConsistent;
	if (ordering != nullptr) {
		auto ord = ordering->emit(ctx);
		if (not ord->get_ir_type()->is_text()) {
			ctx->Error("Expected an expression of type " + ir::TextType::get(ctx->irCtx, false)->to_string() +
			               " here, but got an expression of type " + ctx->color(ord->get_ir_type()->to_string()) +
			               " instead",
			           fileRange);
		}
		auto str = ir::TextType::value_to_string(ord);
		order    = AtomicOperations::parse_atomic_ordering(str, ordering->fileRange, ctx);
	}
	load->setAtomic(order);
	return ir::Value::get(load, candTy, true)->with_range(fileRange);
}

} // namespace qat::ast
