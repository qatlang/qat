#include "./or_use_value.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/result.hpp"

namespace qat::ast {

ir::Value* OrUseValue::emit(EmitCtx* ctx) {
	if (not ctx->fn) {
		ctx->Error("This expression cannot be used outside a function", fileRange);
	}
	auto exp   = expression->emit(ctx);
	auto expTy = exp->get_pass_type();
	exp        = ir::Logic::handle_pass_semantics(ctx, expTy, exp, expression->fileRange);
	if (expTy->is_maybe() || (expTy->is_ref() && expTy->as_ref()->get_subtype()->is_maybe())) {
		auto         mTy       = expTy->is_ref() ? expTy->as_ref()->get_subtype()->as_maybe() : expTy->as_maybe();
		llvm::Value* condition = nullptr;
		if (expTy->is_ref()) {
			condition = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), exp->get_llvm(), 0u));
		} else {
			condition = ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {0u});
		}
		auto currBlock  = ctx->fn->get_block();
		auto trueBlock  = ir::Block::create(ctx->fn, currBlock);
		auto falseBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock  = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		llvm::Value* trueValue = nullptr;
		if (expTy->is_ref()) {
			trueValue = ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), exp->get_llvm(), 1u);
		} else {
			trueValue = ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {1u});
		}
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto cand           = candidate->emit(ctx);
		auto candTy         = cand->get_pass_type();
		cand                = ir::Logic::handle_pass_semantics(ctx, candTy, cand, candidate->fileRange);
		auto expectedCandTy = expTy->is_ref()
		                          ? ir::RefType::get(expTy->as_ref()->has_variability(), mTy->get_subtype(), ctx->irCtx)
		                          : mTy->get_subtype();
		auto otherCandTy    = (expTy->is_ref() && (not expTy->as_ref()->has_variability()))
		                          ? ir::RefType::get(true, mTy->get_subtype(), ctx->irCtx)
		                          : mTy->get_subtype();
		if (not candTy->is_same(expectedCandTy) && not candTy->is_same(otherCandTy)) {
			if (expTy->is_ref()) {
				ctx->Error("The main expression is of type " + ctx->color(expTy->to_string()) +
				               ", which is a reference, so the value to be used if there is no value in the main"
				               " expression, is also expected to be a reference. Expected an expression of type " +
				               ctx->color(expectedCandTy->to_string()) +
				               ((not expectedCandTy->is_same(otherCandTy)) ? (" or " + otherCandTy->to_string()) : "") +
				               " here. Got an expression of type " + ctx->color(candTy->to_string()) + " instead",
				           candidate->fileRange);
			} else {
				ctx->Error("Expected an expression of type " + ctx->color(expectedCandTy->to_string()) +
				               " here. Got an expression of type " + ctx->color(candTy->to_string()) + " instead",
				           candidate->fileRange);
			}
		}
		auto falseResBlock = ctx->fn->get_block();
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		auto phi = ctx->irCtx->builder.CreatePHI(candTy->get_llvm_type(), 2u);
		phi->addIncoming(trueValue, trueBlock->get_bb());
		phi->addIncoming(cand->get_llvm(), falseResBlock->get_bb());
		return ir::Value::get(phi, candTy, not candTy->is_ref());
	} else if (expTy->is_ptr()) {
		auto ptrTy = expTy->as_ptr();
		if (ptrTy->is_non_nullable()) {
			ctx->Error("The main expression is of type " + ctx->color(ptrTy->to_string()) +
			               ", which is a non-nullable pointer type, so it does not make sense to provide"
			               " a value to be used if the main expression is null, as it can never be null",
			           fileRange);
		}
		llvm::Value* condition = nullptr;
		auto         ptrDiffTy = llvm::Type::getIntNTy(
            ctx->irCtx->llctx, ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getPtrDiffType(
                                   ctx->irCtx->get_language_address_space())));
		if (ptrTy->is_multi()) {
			condition = ctx->irCtx->builder.CreateICmpNE(
			    ctx->irCtx->builder.CreatePtrDiff(
			        llvm::Type::getInt8Ty(ctx->irCtx->llctx),
			        ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {1u}),
			        llvm::ConstantPointerNull::get(llvm::PointerType::get(
			            ctx->irCtx->llctx, ptrTy->get_address_space().has_value()
			                                   ? ptrTy->get_address_space().value().get_number(ctx->irCtx)
			                                   : ctx->irCtx->dataLayout.getProgramAddressSpace()))),
			    llvm::ConstantInt::get(ptrDiffTy, 0u, true));
		} else {
			condition = ctx->irCtx->builder.CreateICmpNE(
			    ctx->irCtx->builder.CreatePtrDiff(
			        llvm::Type::getInt8Ty(ctx->irCtx->llctx), exp->get_llvm(),
			        llvm::ConstantPointerNull::get(llvm::PointerType::get(
			            ctx->irCtx->llctx, ptrTy->get_address_space().has_value()
			                                   ? ptrTy->get_address_space().value().get_number(ctx->irCtx)
			                                   : ctx->irCtx->dataLayout.getProgramAddressSpace()))),
			    llvm::ConstantInt::get(ptrDiffTy, 0u, true));
		}
		auto currBlock  = ctx->fn->get_block();
		auto trueBlock  = ir::Block::create(ctx->fn, currBlock);
		auto falseBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock  = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		auto expectedCandTy =
		    ir::PtrType::get(ptrTy->is_subtype_variable(), ptrTy->get_subtype(), true, ptrTy->get_owner(),
		                     ptrTy->is_multi(), ptrTy->get_address_space(), ctx->irCtx);
		llvm::Value* trueValue = nullptr;
		if (ptrTy->is_multi()) {
			trueValue = ctx->irCtx->builder.CreateBitCast(exp->get_llvm(), expectedCandTy->get_llvm_type());
		} else {
			trueValue = exp->get_llvm();
		}
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto cand   = candidate->emit(ctx);
		auto candTy = cand->get_pass_type();
		cand        = ir::Logic::handle_pass_semantics(ctx, candTy, cand, candidate->fileRange);
		if (not candTy->is_same(expectedCandTy)) {
			ctx->Error(
			    "The main expression is of type " + ctx->color(expTy->to_string()) +
			        ", so expected an expression of type " + ctx->color(expectedCandTy->to_string()) +
			        " to be provided here as the value to be used if the main expression is a null pointer. Got an expression of type " +
			        ctx->color(candTy->to_string()) + " instead",
			    candidate->fileRange);
		}
		auto falseResBlock = ctx->fn->get_block();
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		auto phi = ctx->irCtx->builder.CreatePHI(expectedCandTy->get_llvm_type(), 2u);
		phi->addIncoming(trueValue, trueBlock->get_bb());
		phi->addIncoming(cand->get_llvm(), falseResBlock->get_bb());
		return ir::Value::get(phi, expectedCandTy, true);
	} else if (expTy->is_result() && (expTy->is_ref() && expTy->as_ref()->get_subtype()->is_result())) {
		auto         rTy       = expTy->is_ref() ? expTy->as_ref()->get_subtype()->as_result() : expTy->as_result();
		llvm::Value* condition = nullptr;
		if (expTy->is_ref()) {
			condition = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(rTy->get_llvm_type(), exp->get_llvm(), 0u));
		} else {
			condition = ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {0u});
		}
		auto currBlock  = ctx->fn->get_block();
		auto trueBlock  = ir::Block::create(ctx->fn, currBlock);
		auto falseBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock  = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		llvm::Value* trueValue = nullptr;
		if (expTy->is_ref()) {
			trueValue = ctx->irCtx->builder.CreateStructGEP(rTy->get_llvm_type(), exp->get_llvm(), 1u);
		} else {
			trueValue = ctx->irCtx->builder.CreateTruncOrBitCast(
			    ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {1u}), rTy->get_valid_type()->get_llvm_type());
		}
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto cand   = candidate->emit(ctx);
		auto candTy = cand->get_pass_type();
		cand        = ir::Logic::handle_pass_semantics(ctx, candTy, cand, candidate->fileRange);
		auto expectedCandTy =
		    expTy->is_ref() ? ir::RefType::get(expTy->as_ref()->has_variability(), rTy->get_valid_type(), ctx->irCtx)
		                    : rTy->get_valid_type();
		auto otherCandTy = (expTy->is_ref() && (not expTy->as_ref()->has_variability()))
		                       ? ir::RefType::get(true, rTy->get_valid_type(), ctx->irCtx)
		                       : rTy->get_valid_type();
		if (not candTy->is_same(expectedCandTy) && not candTy->is_same(otherCandTy)) {
			if (expTy->is_ref()) {
				ctx->Error("The main expression is of type " + ctx->color(expTy->to_string()) +
				               ", which is a reference, so the value to be used if there is no valid value in the main"
				               " expression, is also expected to be a reference. Expected an expression of type " +
				               ctx->color(expectedCandTy->to_string()) +
				               ((not expectedCandTy->is_same(otherCandTy)) ? (" or " + otherCandTy->to_string()) : "") +
				               " here. Got an expression of type " + ctx->color(candTy->to_string()) + " instead",
				           candidate->fileRange);
			} else {
				ctx->Error("Expected an expression of type " + ctx->color(expectedCandTy->to_string()) +
				               " here. Got an expression of type " + ctx->color(candTy->to_string()) + " instead",
				           candidate->fileRange);
			}
		}
		auto falseResBlock = ctx->fn->get_block();
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		auto phi = ctx->irCtx->builder.CreatePHI(candTy->get_llvm_type(), 2u);
		phi->addIncoming(trueValue, trueBlock->get_bb());
		phi->addIncoming(cand->get_llvm(), falseResBlock->get_bb());
		return ir::Value::get(phi, candTy, not candTy->is_ref());
	} else {
		ctx->Error("Found an expression of type " + ctx->color(expTy->to_string()) +
		               " here, which is not supported for providing alternate values to be used if"
		               " there is no valid value in them. Expected either a value or reference to"
		               " a maybe type, a nullable pointer type, or a value or reference to a result type",
		           fileRange);
		std::unreachable();
	}
}

} // namespace qat::ast
