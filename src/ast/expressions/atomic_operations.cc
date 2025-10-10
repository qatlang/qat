#include "./atomic_operations.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ir::Value* AtomicOperations::emit(EmitCtx* ctx) {
	auto cand     = candidate->emit(ctx);
	auto candTy   = cand->get_ir_type();
	auto isRefVar = cand->has_variability();
	if (candTy->is_ref()) {
		cand->load_ghost_ref(ctx->irCtx->builder);
		isRefVar = candTy->as_ref()->has_variability();
		candTy   = candTy->as_ref()->get_subtype();
	} else if (not cand->is_ghost_ref()) {
		ctx->Error("Expected a reference or a reference-like expression here, got a value of type " +
		               ctx->color(candTy->to_string()),
		           fileRange);
	}
	auto order = llvm::AtomicOrdering::SequentiallyConsistent;
	if (not ordering.empty()) {
		auto ord = ordering[0]->emit(ctx);
		if (not ord->get_ir_type()->is_text()) {
			ctx->Error("Expected an expression of type " + ir::TextType::get(ctx->irCtx, false)->to_string() +
			               " here, but got an expression of type " + ctx->color(ord->get_ir_type()->to_string()) +
			               " instead",
			           fileRange);
		}
		auto str = ir::TextType::value_to_string(ord);
		order    = parse_atomic_ordering(str, ordering[0]->fileRange, ctx);
	}
	auto orderCheck = [](Vec<PrerunExpression*> const& ordering, EmitCtx* ctx) {
		if (ordering.size() > 1u) {
			ctx->Error("Expected only one atomic ordering here for this atomic operation, but got " +
			               std::to_string(ordering.size()) + " values instead",
			           FileRange::merge(ordering.front()->fileRange, ordering.back()->fileRange));
		}
	};
	switch (ops) {
		case AtomicOps::EXCHANGE: {
			if (not isRefVar) {
				ctx->Error("This " + String(cand->get_ir_type()->is_ref() ? "reference" : "reference-like expression") +
				               " does not have variability and hence the atomic exchange operation cannot be used",
				           fileRange);
			}
			orderCheck(ordering, ctx);
			if (arguments.size() != 1u) {
				ctx->Error("The atomic exchange operation technically requires 1 argument to be provided, but got " +
				               (arguments.empty() ? "no" : std::to_string(arguments.size())) + " arguments instead",
				           fileRange);
			}
			auto value = arguments[0]->emit(ctx);
			value      = ir::Logic::handle_pass_semantics(ctx, value->get_pass_type(), value, arguments[0]->fileRange);
			if (not value->get_ir_type()->is_same(candTy)) {
				ctx->Error("The value to be exchanged is expected to be of type " + ctx->color(candTy->to_string()) +
				               " but got an expression of type " + ctx->color(value->get_ir_type()->to_string()) +
				               " instead",
				           arguments[0]->fileRange);
			}
			return ir::Value::get(ctx->irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::BinOp::Xchg,
			                                                          cand->get_llvm(), value->get_llvm(), None, order),
			                      candTy, true);
		}
		case AtomicOps::COMPARE_AND_EXCHANGE: {
			if (not isRefVar) {
				ctx->Error(
				    "This " + String(cand->get_ir_type()->is_ref() ? "reference" : "reference-like expression") +
				        " does not have variability and hence the atomic compare-and-exchange operation cannot be used",
				    fileRange);
			}
			auto failureOrder = llvm::AtomicOrdering::SequentiallyConsistent;
			if (ordering.size() > 2u) {
				ctx->Error("The atomic compare-and-exchange operation can have atmost two atomic orderings, but got " +
				               std::to_string(ordering.size()) + " instead",
				           FileRange::merge(ordering.front()->fileRange, ordering.back()->fileRange));
			} else if (ordering.size() == 2u) {
				auto ord = ordering[1]->emit(ctx);
				if (not ord->get_ir_type()->is_text()) {
					ctx->Error("Expected an expression of type " + ir::TextType::get(ctx->irCtx, false)->to_string() +
					               " here, but got an expression of type " +
					               ctx->color(ord->get_ir_type()->to_string()) + " instead",
					           fileRange);
					auto str     = ir::TextType::value_to_string(ord);
					failureOrder = parse_atomic_ordering(str, ordering[1]->fileRange, ctx);
				}
			} else if (not ordering.empty()) {
				failureOrder = order;
				ctx->irCtx->Warning(
				    "The compare-and-exchange atomic operation technically requires 2 atomic orderings to be"
				    " provided. The first one is the success ordering to be used if the comparison is true, and"
				    " the second one is the failure ordering to be used if the comparison is false. Providing"
				    " only one atomic ordering means that the same ordering will be used in both scenarios",
				    ordering[0]->fileRange);
			}
			if (arguments.size() != 2u) {
				ctx->Error(
				    "The compare-and-exchange atomic operation requires exactly two arguments to be provided, but got " +
				        (arguments.empty() ? "no" : std::to_string(arguments.size())) + " arguments instead",
				    fileRange);
			}
			auto condition = arguments[0]->emit(ctx);
			condition =
			    ir::Logic::handle_pass_semantics(ctx, condition->get_pass_type(), condition, arguments[0]->fileRange);
			if (not condition->get_ir_type()->is_bool()) {
				ctx->Error("Expected a value of type " + ctx->color("bool") + " here, but got an expression of type " +
				               ctx->color(condition->get_ir_type()->to_string()) + " instead",
				           arguments[0]->fileRange);
			}
			auto value = arguments[1]->emit(ctx);
			value      = ir::Logic::handle_pass_semantics(ctx, value->get_pass_type(), value, arguments[1]->fileRange);
			if (not value->get_ir_type()->is_same(candTy)) {
				ctx->Error("The value to be exchanged is expected to be of type " + ctx->color(candTy->to_string()) +
				               ", but got an expression of type " + ctx->color(value->get_ir_type()->to_string()) +
				               " instead",
				           arguments[1]->fileRange);
			}
			return ir::Value::get(ctx->irCtx->builder.CreateAtomicCmpXchg(cand->get_llvm(), condition->get_llvm(),
			                                                              value->get_llvm(), None, order, failureOrder),
			                      candTy, false);
		}
	}
}

} // namespace qat::ast
