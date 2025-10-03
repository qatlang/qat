#include "./atomic_copy.hpp"
#include "../../IR/logic.hpp"

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
		auto str = ir::TextType::value_to_string(ordering->emit(ctx));
		if (str == "unordered") {
			order = llvm::AtomicOrdering::Unordered;
		} else if (str == "relaxed") {
			order = llvm::AtomicOrdering::Monotonic;
		} else if (str == "acquire") {
			order = llvm::AtomicOrdering::Acquire;
		} else if (str == "release") {
			order = llvm::AtomicOrdering::Release;
		} else if (str == "acquire_release") {
			order = llvm::AtomicOrdering::AcquireRelease;
		} else if (str == "sequentially_consistent") {
			order = llvm::AtomicOrdering::SequentiallyConsistent;
		} else {
			ctx->Error("Unexpected atomic ordering " + ctx->color(str) + " found here", ordering->fileRange);
		}
	}
	load->setAtomic(order);
	return ir::Value::get(load, candTy, true)->with_range(fileRange);
}

} // namespace qat::ast
