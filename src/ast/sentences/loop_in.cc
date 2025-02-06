#include "./loop_in.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../../IR/types/vector.hpp"
#include "../expression.hpp"

#include <llvm/IR/Instructions.h>

namespace qat::ast {

ir::Value* LoopIn::emit(EmitCtx* ctx) {
	for (auto& loopInfo : ctx->loopsInfo) {
		if (loopInfo.name.has_value()) {
			if (loopInfo.name->value == itemName.value) {
				ctx->Error("The tag for the loop item provided here is already used by another loop", itemName.range,
				           Pair<String, FileRange>{"The existing tag was found here", loopInfo.name->range});
			}
			if (indexName.has_value() && (loopInfo.name->value == indexName->value)) {
				ctx->Error("The tag for the loop index provided here is already used by another loop", indexName->range,
				           Pair<String, FileRange>{"The existing tag was found here", loopInfo.name->range});
			}
		}
	}
	for (auto& brk : ctx->breakables) {
		if (brk.tag.has_value()) {
			if (brk.tag->value == itemName.value) {
				ctx->Error("The name for the loop item provided here is already used by another " +
				               ctx->color(brk.type_to_string()),
				           itemName.range, Pair<String, FileRange>{"The existing tag was found here", brk.tag->range});
			}
			if (indexName.has_value() && (brk.tag->value == indexName->value)) {
				ctx->Error("The name for the loop index provided here is already used by another " +
				               ctx->color(brk.type_to_string()),
				           indexName->range,
				           Pair<String, FileRange>{"The existing tag was found here", brk.tag->range});
			}
		}
	}
	auto block = ctx->get_fn()->get_block();
	if (block->has_value(itemName.value)) {
		ctx->Error("There already exists another local value with the same name as this tag", itemName.range,
		           block->get_value(itemName.value)->has_associated_range()
		               ? Maybe<Pair<String, FileRange>>(
		                     {"The local value was found here", block->get_value(itemName.value)->get_file_range()})
		               : None);
	}
	if (indexName.has_value() && block->has_value(indexName->value)) {
		ctx->Error("There already exists another local value with the same name as this tag", indexName->range,
		           block->get_value(indexName->value)->has_associated_range()
		               ? Maybe<Pair<String, FileRange>>(
		                     {"The local value was found here", block->get_value(indexName->value)->get_file_range()})
		               : None);
	}
	auto       candExp    = candidate->emit(ctx);
	bool       isRefUnder = candExp->get_ir_type()->is_reference() || candExp->is_local_value();
	bool       candHasVar = false;
	auto       candType = candExp->get_ir_type()->is_reference() ? candExp->get_ir_type()->as_reference()->get_subtype()
	                                                             : candExp->get_ir_type();
	auto const isTyArray    = candType->is_array();
	auto const isTySlice    = candType->is_mark() && candType->as_mark()->is_slice();
	auto const isTyCString  = candType->is_native_type() && candType->as_native_type()->is_cstring();
	auto const isTyStrSlice = candType->is_string_slice();
	auto const isTyVec      = candType->is_vector();
	if (candExp->get_ir_type()->is_reference()) {
		candExp->load_ghost_reference(ctx->irCtx->builder);
	} else if (candExp->is_ghost_reference()) {
		isRefUnder = true;
	}
	if (isTyArray || isTySlice || isTyCString || isTyStrSlice || isTyVec) {
		ir::Type*    elemTy  = nullptr;
		ir::Type*    countTy = ir::NativeType::get_usize(ctx->irCtx);
		llvm::Value* ptrVal  = nullptr;
		llvm::Value* lenVal  = nullptr;
		if (isTyArray) {
			elemTy  = candType->as_array()->get_element_type();
			countTy = ir::UnsignedType::create(64u, ctx->irCtx);
			if (not isRefUnder) {
				candExp    = candExp->make_local(ctx, None, candidate->fileRange);
				isRefUnder = true;
				candHasVar = true;
			}
			lenVal = llvm::ConstantInt::get(countTy->get_llvm_type(), candType->as_array()->get_length(), false);
		} else if (isTySlice) {
			elemTy = candType->as_mark()->get_subtype();
			if (isRefUnder) {
				ptrVal = ctx->irCtx->builder.CreateLoad(
				    llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout->getProgramAddressSpace()),
				    ctx->irCtx->builder.CreateStructGEP(candType->get_llvm_type(), candExp->get_llvm(), 0u));
				lenVal = ctx->irCtx->builder.CreateLoad(
				    countTy->get_llvm_type(),
				    ctx->irCtx->builder.CreateStructGEP(candType->get_llvm_type(), candExp->get_llvm(), 1u));
			} else {
				ptrVal = ctx->irCtx->builder.CreateExtractValue(candExp->get_llvm(), {0u});
				lenVal = ctx->irCtx->builder.CreateExtractValue(candExp->get_llvm(), {1u});
			}
			candHasVar = candType->as_mark()->is_subtype_variable();
		} else if (isTyCString) {
			elemTy = ir::UnsignedType::create(8, ctx->irCtx);
			if (isRefUnder) {
				candExp = ir::Value::get(ctx->irCtx->builder.CreateLoad(candType->get_llvm_type(), candExp->get_llvm()),
				                         candType, false);
			}
			ptrVal = candExp->get_llvm();
		} else if (isTyStrSlice) {
			elemTy = ir::UnsignedType::create(8, ctx->irCtx);
			if (isRefUnder) {
				ptrVal = ctx->irCtx->builder.CreateLoad(
				    llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout->getProgramAddressSpace()),
				    ctx->irCtx->builder.CreateStructGEP(candType->get_llvm_type(), candExp->get_llvm(), 0u));
				lenVal = ctx->irCtx->builder.CreateLoad(
				    countTy->get_llvm_type(),
				    ctx->irCtx->builder.CreateStructGEP(candType->get_llvm_type(), candExp->get_llvm(), 1u));
			} else {
				ptrVal = ctx->irCtx->builder.CreateExtractValue(candExp->get_llvm(), {0u});
				lenVal = ctx->irCtx->builder.CreateExtractValue(candExp->get_llvm(), {1u});
			}
		} else if (isTyVec) {
			elemTy = candType->as_vector()->get_element_type();
			if (isRefUnder) {
				candExp = ir::Value::get(ctx->irCtx->builder.CreateLoad(elemTy->get_llvm_type(), candExp->get_llvm()),
				                         candType, true);
			}
			if (candType->as_vector()->is_fixed()) {
				lenVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
				                                candType->as_vector()->get_count());
			} else {
				lenVal = ctx->irCtx->builder.CreateMul(ctx->mod->link_intrinsic(ir::IntrinsicID::vectorScale),
				                                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
				                                                              candType->as_vector()->get_count()));
			}
		}
		auto currBlock = ctx->get_fn()->get_block();
		auto mainBlock = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
		mainBlock->link_previous_block(currBlock);
		auto loopBlock = ir::Block::create(ctx->get_fn(), mainBlock);
		auto condBlock = ir::Block::create(ctx->get_fn(), mainBlock);
		condBlock->link_previous_block(loopBlock);
		auto trueBlock = ir::Block::create(ctx->get_fn(), mainBlock);
		trueBlock->link_previous_block(condBlock);
		auto restBlock = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
		restBlock->link_previous_block(mainBlock);
		// Adding loop info
		ctx->loopsInfo.push_back(LoopInfo(itemName, mainBlock, condBlock, restBlock, nullptr, LoopType::OVER));
		//
		(void)ir::add_branch(ctx->irCtx->builder, mainBlock->get_bb());
		mainBlock->set_active(ctx->irCtx->builder);
		auto zeroU8    = llvm::ConstantInt::get(llvm::IntegerType::getInt8Ty(ctx->irCtx->llctx), 0u, false);
		auto zero64    = llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(ctx->irCtx->llctx), 0u);
		auto indexVar  = mainBlock->new_value(indexName.has_value() ? indexName->value : utils::uid_string(), countTy,
		                                     false, indexName.has_value() ? indexName->range : fileRange);
		auto elemUseTy = elemTy;
		if (not isTyVec) {
			elemUseTy = ir::ReferenceType::get(candHasVar, elemTy, ctx->irCtx);
		}
		auto itemVar = mainBlock->new_value(itemName.value, elemUseTy, false, itemName.range);
		ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(countTy->get_llvm_type(), 0u, false),
		                                indexVar->get_llvm());
		ctx->irCtx->builder.CreateCondBr(
		    isTyCString
		        ? ctx->irCtx->builder.CreateAnd(
		              ctx->irCtx->builder.CreateICmpNE(
		                  ctx->irCtx->builder.CreatePtrDiff(
		                      llvm::IntegerType::getInt8Ty(ctx->irCtx->llctx), candExp->get_llvm(),
		                      llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(candType->get_llvm_type()))),
		                  llvm::ConstantInt::get(ir::NativeType::get_ptrdiff_unsigned(ctx->irCtx)->get_llvm_type(), 0,
		                                         false)),
		              ctx->irCtx->builder.CreateICmpNE(
		                  ctx->irCtx->builder.CreateLoad(llvm::IntegerType::getInt8Ty(ctx->irCtx->llctx),
		                                                 candExp->get_llvm()),
		                  zeroU8))
		        : ctx->irCtx->builder.CreateICmpULT(llvm::ConstantInt::get(countTy->get_llvm_type(), 0u, false),
		                                            lenVal),
		    loopBlock->get_bb(), restBlock->get_bb());
		// Loop block begins
		loopBlock->set_active(ctx->irCtx->builder);
		llvm::Value* llItemVal = nullptr;
		if (isTyArray) {
			llItemVal = ctx->irCtx->builder.CreateInBoundsGEP(
			    elemTy->get_llvm_type(), candExp->get_llvm(),
			    {zero64,
			     ctx->irCtx->builder.CreateLoad(indexVar->get_ir_type()->get_llvm_type(), indexVar->get_llvm())});
		} else if (isTyVec) {
			llItemVal = ctx->irCtx->builder.CreateExtractElement(
			    candExp->get_llvm(),
			    ctx->irCtx->builder.CreateLoad(indexVar->get_ir_type()->get_llvm_type(), indexVar->get_llvm()));
		} else {
			llItemVal = ctx->irCtx->builder.CreateInBoundsGEP(
			    elemTy->get_llvm_type(), ptrVal,
			    {ctx->irCtx->builder.CreateLoad(indexVar->get_ir_type()->get_llvm_type(), indexVar->get_llvm())});
		}
		ctx->irCtx->builder.CreateStore(llItemVal, itemVar->get_llvm());
		emit_sentences(sentences, ctx); // All sentences being emitted
		loopBlock->destroy_locals(ctx);
		(void)ir::add_branch(ctx->irCtx->builder, condBlock->get_bb());
		condBlock->set_active(ctx->irCtx->builder);
		// Condition
		llvm::Value* loopCond = nullptr;
		if (isTyCString) {
			loopCond = ctx->irCtx->builder.CreateICmpNE(
			    ctx->irCtx->builder.CreateLoad(
			        zeroU8->getType(), ctx->irCtx->builder.CreateInBoundsGEP(
			                               zeroU8->getType(), candExp->get_llvm(),
			                               {ctx->irCtx->builder.CreateAdd(
			                                   ctx->irCtx->builder.CreateLoad(zero64->getType(), indexVar->get_llvm()),
			                                   llvm::ConstantInt::get(zero64->getType(), 1u, false))})),
			    zeroU8);
		} else {
			loopCond = ctx->irCtx->builder.CreateICmpULT(
			    ctx->irCtx->builder.CreateAdd(
			        ctx->irCtx->builder.CreateLoad(countTy->get_llvm_type(), indexVar->get_llvm()),
			        llvm::ConstantInt::get(zero64->getType(), 1u, false)),
			    lenVal);
		}
		ctx->irCtx->builder.CreateCondBr(loopCond, trueBlock->get_bb(), restBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		ctx->irCtx->builder.CreateStore(
		    ctx->irCtx->builder.CreateAdd(
		        ctx->irCtx->builder.CreateLoad(indexVar->get_ir_type()->get_llvm_type(), indexVar->get_llvm()),
		        llvm::ConstantInt::get(indexVar->get_ir_type()->get_llvm_type(), 1u, false)),
		    indexVar->get_llvm());
		(void)ir::add_branch(ctx->irCtx->builder, loopBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
	} else {
		ctx->Error("Looping over elements is not supported for an expression of type " +
		               ctx->color(candExp->get_ir_type()->to_string()),
		           fileRange);
	}
	return nullptr;
}

} // namespace qat::ast
