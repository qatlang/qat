#include "./end_pointer.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* EndPointer::emit(EmitCtx* ctx) {
	auto* cand = candidate->emit(ctx);
	cand       = ir::Logic::handle_pass_semantics(ctx, cand->get_pass_type(), cand, fileRange);
	if (cand->get_ir_type()->is_ptr()) {
		auto ptrTy = cand->get_ir_type()->as_ptr();
		if (not ptrTy->get_owner().is_heap()) {
			ctx->Error(
			    "Calling destructors on pointers and multi-pointers are only supported for heap ownership, but got a pointer value of type " +
			        ctx->color(ptrTy->to_string()) + " instead",
			    fileRange);
		}
		auto subTy = ptrTy->get_subtype();
		if (subTy->is_destructible()) {
			if (ptrTy->is_multi()) {
				if (kind == EndPointerKind::PTR) {
					ctx->Error("Invalid syntax for destroying objects in the multi-pointer type " +
					               ctx->color(ptrTy->to_string()) + ". Use " + ctx->color("'end:multi()") + ", " +
					               ctx->color("'end:from(firstIndexInclusive)") + ", " +
					               ctx->color("'end:to(lastIndexExclusive)") + " or " +
					               ctx->color("'end:in(firstIndex, lastIndexExclusive)") + " for multi-pointers",
					           fileRange);
				}
				const auto* index   = ctx->get_fn()->get_str_comparison_index(ctx->irCtx);
				const auto  indexTy = index->get_ir_type()->get_llvm_type();
				const auto  ptrVal  = ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {0u});
				const auto  length  = ctx->irCtx->builder.CreateExtractValue(cand->get_llvm(), {1u});
				if (kind == EndPointerKind::MULTI) {
					ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(indexTy, 0u), index->get_llvm());
				} else if (kind == EndPointerKind::FROM || kind == EndPointerKind::RANGE) {
					auto fromInd = args[0]->emit(ctx);
					fromInd =
					    ir::Logic::handle_pass_semantics(ctx, fromInd->get_pass_type(), fromInd, args[0]->fileRange);
					if (not fromInd->get_ir_type()->is_native_type() &&
					    not fromInd->get_ir_type()->as_native_type()->is_native_usize()) {
						ctx->Error("Expected a value of type " + ctx->color("usize") +
						               " here for the inclusive starting index, but got an expression of type " +
						               ctx->color(fromInd->get_ir_type()->to_string()) + " instead",
						           args[0]->fileRange);
					}
					ctx->irCtx->builder.CreateStore(fromInd->get_llvm(), index->get_llvm());
					const auto currBlock    = ctx->get_fn()->get_block();
					const auto indFailBlock = ir::Block::create(ctx->get_fn(), currBlock);
					const auto restBlock    = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
					restBlock->link_previous_block(currBlock);
					ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGE(fromInd->get_llvm(), length),
					                                 indFailBlock->get_bb(), restBlock->get_bb());
					indFailBlock->set_active(ctx->irCtx->builder);
					ir::Logic::panic_in_function(
					    ctx->get_fn(),
					    {
					        ir::TextType::create_value(
					            ctx->irCtx, ctx->mod,
					            "The inclusive starting index given in " +
					                String(kind == EndPointerKind::FROM ? "'end:from" : "'end:in") +
					                " to destroy objects in the multi-pointer is "),
					        fromInd,
					        ir::TextType::create_value(
					            ctx->irCtx, ctx->mod,
					            ", which is not less than the length of the multi-pointer. The length is "),
					        ir::Value::get(length, ir::NativeType::get_usize(ctx->irCtx), true),
					    },
					    {}, args[0]->fileRange, ctx);
					restBlock->set_active(ctx->irCtx->builder);
				}
				// NOTE - DO NOT MERGE THE BELOW CONDITION TO THE ABOVE if-else AS BOTH HANDLES THE 'end:in CASE
				ir::Value* toInd = nullptr;
				if (kind == EndPointerKind::TO || kind == EndPointerKind::RANGE) {
					auto relArg = kind == EndPointerKind::TO ? args[0] : args[1];
					toInd       = relArg->emit(ctx);
					toInd = ir::Logic::handle_pass_semantics(ctx, toInd->get_pass_type(), toInd, relArg->fileRange);
					if (not toInd->get_ir_type()->is_native_type() &&
					    not toInd->get_ir_type()->as_native_type()->is_native_usize()) {
						ctx->Error("Expected a value of type " + ctx->color("usize") +
						               " here for the exclusive end index, but got an expression of type " +
						               ctx->color(toInd->get_ir_type()->to_string()) + " instead",
						           relArg->fileRange);
					}
					const auto currBlock    = ctx->get_fn()->get_block();
					const auto indFailBlock = ir::Block::create(ctx->get_fn(), currBlock);
					const auto restBlock    = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
					restBlock->link_previous_block(currBlock);
					ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGT(toInd->get_llvm(), length),
					                                 indFailBlock->get_bb(), restBlock->get_bb());
					indFailBlock->set_active(ctx->irCtx->builder);
					ir::Logic::panic_in_function(
					    ctx->get_fn(),
					    {
					        ir::TextType::create_value(ctx->irCtx, ctx->mod,
					                                   "The exclusive ending index given in " +
					                                       String(kind == EndPointerKind::TO ? "'end:to" : "'end:in") +
					                                       " to destroy objects in the multi-pointer is "),
					        toInd,
					        ir::TextType::create_value(
					            ctx->irCtx, ctx->mod,
					            ", which is greater than the length of the multi-pointer. The length is "),
					        ir::Value::get(length, ir::NativeType::get_usize(ctx->irCtx), true),
					    },
					    {}, relArg->fileRange, ctx);
					restBlock->set_active(ctx->irCtx->builder);
				}
				const auto currBlock = ctx->get_fn()->get_block();
				const auto mainBlock = ir::Block::create(ctx->get_fn(), currBlock);
				const auto restBlock = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
				restBlock->link_previous_block(currBlock);
				const auto intPtrTy =
				    llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->clangTargetInfo->getIntPtrType());
				const auto startIndex      = (kind == EndPointerKind::MULTI || kind == EndPointerKind::TO)
				                                 ? llvm::cast<llvm::Value>(llvm::ConstantInt::get(indexTy, 0u, false))
				                                 : ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm());
				auto       lenCheckInitial = ctx->irCtx->builder.CreateICmpULT(startIndex, length);
				if (kind == EndPointerKind::TO || kind == EndPointerKind::RANGE) {
					lenCheckInitial = ctx->irCtx->builder.CreateAnd(
					    ctx->irCtx->builder.CreateICmpULT(startIndex, toInd->get_llvm()), lenCheckInitial);
				}
				ctx->irCtx->builder.CreateCondBr(
				    ptrTy->is_nullable()
				        ? ctx->irCtx->builder.CreateAnd(
				              ctx->irCtx->builder.CreateICmpNE(ctx->irCtx->builder.CreatePtrToInt(ptrVal, intPtrTy),
				                                               llvm::ConstantInt::get(intPtrTy, 0u, false)),
				              lenCheckInitial)
				        : lenCheckInitial,
				    mainBlock->get_bb(), restBlock->get_bb());
				mainBlock->set_active(ctx->irCtx->builder);
				subTy->destroy_value(
				    ctx->irCtx,
				    ir::Value::get(ctx->irCtx->builder.CreateInBoundsGEP(
				                       subTy->get_llvm_type(), ptrVal,
				                       {ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm())}),
				                   ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx),
				                   false),
				    ctx->get_fn());
				ctx->irCtx->builder.CreateStore(
				    ctx->irCtx->builder.CreateAdd(ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm()),
				                                  llvm::ConstantInt::get(indexTy, 1u, false)),
				    index->get_llvm());
				auto indexLoad = ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm());
				auto lenCheck  = ctx->irCtx->builder.CreateICmpULT(indexLoad, length);
				if (kind == EndPointerKind::TO || kind == EndPointerKind::RANGE) {
					lenCheck = ctx->irCtx->builder.CreateAnd(
					    ctx->irCtx->builder.CreateICmpULT(indexLoad, toInd->get_llvm()), lenCheck);
				}
				ctx->irCtx->builder.CreateCondBr(lenCheck, mainBlock->get_bb(), restBlock->get_bb());
				restBlock->set_active(ctx->irCtx->builder);
			} else {
				if (kind != EndPointerKind::PTR) {
					ctx->Error("Invalid syntax for ending object in the pointer type " +
					               ctx->color(ptrTy->to_string()) +
					               ". This syntax is only supported for multi-pointers. Use " +
					               ctx->color("'end:ptr()") = " instead",
					           fileRange);
				}
				subTy->destroy_value(
				    ctx->irCtx,
				    ir::Value::get(cand->get_llvm(),
				                   ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx),
				                   false),
				    ctx->get_fn());
			}
		}
		return ir::Value::get(llvm::UndefValue::get(llvm::Type::getVoidTy(ctx->irCtx->llctx)),
		                      ir::VoidType::get(ctx->irCtx->llctx), false);
	} else {
		ctx->Error("Expected a pointer value here, but found an expression of type " +
		               ctx->color(cand->get_ir_type()->to_string()) + " here",
		           fileRange);
		std::unreachable();
	}
}

} // namespace qat::ast
