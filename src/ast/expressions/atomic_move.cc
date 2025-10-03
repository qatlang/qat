#include "./atomic_move.hpp"

namespace qat::ast {

ir::Value* AtomicMove::emit(EmitCtx* ctx) {
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
		               " does not have simple-move and hence does not support atomic move",
		           fileRange);
	}
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
	return ir::Value::get(ctx->irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::BinOp::Xchg, cand->get_llvm(),
	                                                          llvm::Constant::getNullValue(candTy->get_llvm_type()),
	                                                          None, order),
	                      candTy, true)
	    ->with_range(fileRange);
}

} // namespace qat::ast
