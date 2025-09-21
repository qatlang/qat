#include "./non_null.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/result.hpp"

#include <clang/Basic/AddressSpaces.h>

namespace qat::ast {

ir::Value* NonNull::emit(EmitCtx* ctx) {
	if (not ctx->has_fn()) {
		ctx->Error("This expression cannot be used outside a function", fileRange);
	}
	auto cand   = candidate->emit(ctx);
	auto candTy = cand->get_pass_type();
	cand        = ir::Logic::handle_pass_semantics(ctx, candTy, cand, candidate->fileRange);
	if (candTy->is_maybe() || (candTy->is_ref() && candTy->as_ref()->get_subtype()->is_maybe())) {
		if (candTy->is_ref()) {
			cand->load_ghost_ref(ctx->irCtx->builder);
		}
		auto         mTy       = candTy->is_ref() ? candTy->as_ref()->get_subtype()->as_maybe() : candTy->as_maybe();
		llvm::Value* condition = nullptr;
		if (candTy->is_ref()) {
			condition = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), cand->get_llvm(), 0u));
		} else {
			condition = ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {0u});
		}
		condition      = ctx->irCtx->builder.CreateNot(condition);
		auto currBlock = ctx->fn->get_block();
		auto trueBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), restBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		ir::Logic::panic_in_function(
		    ctx->fn,
		    {ir::TextType::create_value(
		        ctx->irCtx, ctx->mod,
		        "The maybe expression of type " + mTy->to_string() +
		            " does not contain a value in it, so failed to affirm that this expression has a value of type " +
		            mTy->get_subtype()->to_string())},
		    {}, fileRange, ctx);
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		if (candTy->is_ref()) {
			return ir::Value::get(ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), cand->get_llvm(), 1u),
			                      ir::RefType::get(candTy->as_ref()->has_variability(), mTy->get_subtype(), ctx->irCtx),
			                      false);
		} else {
			return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {1u}), mTy->get_subtype(),
			                      true);
		}
	} else if (candTy->is_ptr()) {
		if (candTy->as_ptr()->is_non_nullable()) {
			ctx->Error("The type of this expression is " + ctx->color(candTy->to_string()) +
			               ", which is a non-nullable " + (candTy->as_ptr()->is_multi() ? "multi-pointer" : "pointer") +
			               " type. It is unnecessary to affirm such an expression to not be null",
			           fileRange);
		}
		auto         ptrTy     = candTy->as_ptr();
		llvm::Value* condition = nullptr;
		auto         ptrDiffTy = llvm::Type::getIntNTy(
            ctx->irCtx->llctx,
            ctx->irCtx->clangTargetInfo->getTypeWidth(ctx->irCtx->clangTargetInfo->getPtrDiffType(
                clang::getLangASFromTargetAS(ptrTy->has_address_space()
		                                                 ? ptrTy->get_address_space().value().get_number(ctx->irCtx)
		                                                 : ctx->irCtx->dataLayout.getProgramAddressSpace()))));
		if (ptrTy->is_multi()) {
			condition = ctx->irCtx->builder.CreateICmpEQ(
			    ctx->irCtx->builder.CreatePtrDiff(
			        llvm::Type::getInt8Ty(ctx->irCtx->llctx),
			        ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {0u}),
			        llvm::ConstantPointerNull::get(llvm::PointerType::get(
			            ctx->irCtx->llctx, ptrTy->has_address_space()
			                                   ? ptrTy->get_address_space().value().get_number(ctx->irCtx)
			                                   : ctx->irCtx->dataLayout.getProgramAddressSpace()))),
			    llvm::ConstantInt::get(ptrDiffTy, 0u, true));
		} else {
			condition = ctx->irCtx->builder.CreateICmpEQ(
			    ctx->irCtx->builder.CreatePtrDiff(
			        llvm::Type::getInt8Ty(ctx->irCtx->llctx), cand->get_llvm(),
			        llvm::ConstantPointerNull::get(llvm::PointerType::get(
			            ctx->irCtx->llctx, ptrTy->get_address_space().has_value()
			                                   ? ptrTy->get_address_space().value().get_number(ctx->irCtx)
			                                   : ctx->irCtx->dataLayout.getProgramAddressSpace()))),
			    llvm::ConstantInt::get(ptrDiffTy, 0u, true));
		}
		auto currBlock = ctx->fn->get_block();
		auto trueBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), restBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		ir::Logic::panic_in_function(
		    ctx->fn,
		    {ir::TextType::create_value(ctx->irCtx, ctx->mod,
		                                "This is a null pointer, so failed to affirm that this pointer is not null")},
		    {}, fileRange, ctx);
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		auto resTy = ir::PtrType::get(ptrTy->is_subtype_variable(), ptrTy->get_subtype(), true, ptrTy->get_owner(),
		                              ptrTy->is_multi(), ptrTy->get_address_space(), ctx->irCtx);
		if (ptrTy->is_multi()) {
			return ir::Value::get(ctx->irCtx->builder.CreateBitCast(cand->get_llvm(), resTy->get_llvm_type()), resTy,
			                      true);
		} else {
			return ir::Value::get(ctx->irCtx->builder.CreatePointerCast(cand->get_llvm(), resTy->get_llvm_type()),
			                      resTy, true);
		}
	} else if (candTy->is_result() || (candTy->is_ref() && candTy->as_ref()->get_subtype()->is_result())) {
		if (candTy->is_ref()) {
			cand->load_ghost_ref(ctx->irCtx->builder);
		}
		auto         rTy       = candTy->is_ref() ? candTy->as_ref()->get_subtype()->as_result() : candTy->as_result();
		llvm::Value* condition = nullptr;
		if (candTy->is_ref()) {
			condition = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(rTy->get_llvm_type(), cand->get_llvm(), 0u));
		} else {
			condition = ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {0u});
		}
		condition      = ctx->irCtx->builder.CreateNot(condition);
		auto currBlock = ctx->fn->get_block();
		auto trueBlock = ir::Block::create(ctx->fn, currBlock);
		auto restBlock = ir::Block::create(ctx->fn, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		ctx->irCtx->builder.CreateCondBr(condition, trueBlock->get_bb(), restBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		ir::Logic::panic_in_function(
		    ctx->fn,
		    {ir::TextType::create_value(ctx->irCtx, ctx->mod,
		                                "This expression of type " + rTy->to_string() + " has an error value of type " +
		                                    rTy->get_error_type()->to_string() + " instead of a valid value")},
		    {}, fileRange, ctx);
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		if (candTy->is_ref()) {
			return ir::Value::get(
			    ctx->irCtx->builder.CreatePointerCast(
			        ctx->irCtx->builder.CreateStructGEP(rTy->get_llvm_type(), cand->get_llvm(), 1u),
			        llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace())),
			    ir::RefType::get(candTy->as_ref()->has_variability(), rTy->get_valid_type(), ctx->irCtx), false);
		} else {
			return ir::Value::get(
			    ctx->irCtx->builder.CreateTruncOrBitCast(
			        ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {1u}),
			        llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getTypeSizeInBits(
			                                                     rTy->get_valid_type()->get_llvm_type()))),
			    rTy->get_valid_type(), true);
		}
	} else {
		ctx->Error("Found an expression of type " + ctx->color(candTy->to_string()) +
		               " here, which is not supported for non-null affirmation. Expected either a value or"
		               " reference to a maybe type, a nullable pointer type, or a value or reference to a result type",
		           fileRange);
	}
	std::unreachable();
}

} // namespace qat::ast
