#include "./in.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/region.hpp"
#include "../types/qat_type.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

void InExpression::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                       EmitCtx* ctx) {
	UPDATE_DEPS(candidate);
	if (is_target_expression()) {
		UPDATE_DEPS(target_as_expression());
	} else if (is_target_region()) {
		UPDATE_DEPS(target_as_region());
	} else if (is_target_type()) {
		UPDATE_DEPS(target_as_type());
	}
}

ir::Value* InExpression::emit(EmitCtx* ctx) {
	ir::MarkType* finalTy = nullptr;
	if (is_type_inferred()) {
		if (not inferredType->is_mark()) {
			ctx->Error("The type inferred from scope is " + ctx->color(inferredType->to_string()) +
			               ", which is not a mark type. This expression expects to be a mark type",
			           fileRange);
		}
		if (inferredType->as_mark()->is_slice()) {
			ctx->Error(
			    "The type inferred from scope is " + ctx->color(inferredType->to_string()) +
			        ", which is a slice type. This expression expects to be a mark type and cannot be of slice type",
			    fileRange);
		}
		finalTy = inferredType->as_mark();
		if (is_target_heap() && not finalTy->get_owner().is_of_heap()) {
			ctx->Error("The type inferred from scope is " + ctx->color(inferredType->to_string()) +
			               ", which is a mark type without heap ownership. This expression expects to be of type " +
			               ctx->color(ir::MarkType::get(finalTy->is_subtype_variable(), finalTy->get_subtype(),
			                                            finalTy->is_non_nullable(), ir::MarkOwner::of_heap(), false,
			                                            ctx->irCtx)
			                              ->to_string()),
			           fileRange);
		} else if (is_target_region() &&
		           not(finalTy->get_owner().is_of_region() || finalTy->get_owner().is_of_any_region())) {
			ctx->Error(
			    "The type inferred from scope is " + ctx->color(inferredType->to_string()) +
			        ", which is a mark type without region ownership. This expression expects either to be of type " +
			        ctx->color(ir::MarkType::get(finalTy->is_subtype_variable(), finalTy->get_subtype(),
			                                     finalTy->is_non_nullable(), ir::MarkOwner::of_any_region(), false,
			                                     ctx->irCtx)
			                       ->to_string()) +
			        " or of type " +
			        ctx->color("mark" + String(finalTy->is_non_nullable() ? "!" : ":") + "[" +
			                   (finalTy->is_subtype_variable() ? "var " : "") + finalTy->get_subtype()->to_string() +
			                   ", region(" + target_as_region()->to_string() + ")]"),
			    fileRange);
		} else {
			ctx->Error("Invalid allocator for in-expression", fileRange);
		}
		if (candidate->has_type_inferrance()) {
			candidate->as_type_inferrable()->set_inference_type(finalTy->get_subtype());
		}
	}
	ir::Value* result = nullptr;
	// FIXME - Maybe this is redundant? No need for startBlock?
	auto startBlock   = ctx->get_fn()->get_block();
	auto nonNullBlock = ir::Block::create(ctx->get_fn(), startBlock);
	auto restBlock    = ir::Block::create(ctx->get_fn(), startBlock->get_parent());
	restBlock->link_previous_block(startBlock);
	nonNullBlock->set_active(ctx->irCtx->builder);
	if (candidate->isInPlaceCreatable()) {
		auto dummyValue =
		    ir::Value::get(llvm::UndefValue::get(llvm::PointerType::get(
		                       ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace())),
		                   ir::RefType::get(true, ir::IntegerType::get(8u, ctx->irCtx), ctx->irCtx), false);
		candidate->asInPlaceCreatable()->setCreateIn(dummyValue);
		candidate->asInPlaceCreatable()->ignoreCreateInType = true;
		auto resExpr                                        = candidate->emit(ctx);
		auto exprTy                                         = resExpr->get_pass_type();

		auto currBlock = ctx->get_fn()->get_block();
		startBlock->set_active(ctx->irCtx->builder);
		if (is_target_heap()) {
			auto mallocName = ctx->mod->link_internal_dependency(ir::InternalDependency::malloc, ctx->irCtx, fileRange);
			auto mallocFn   = ctx->mod->get_llvm_module()->getFunction(mallocName);
			auto mallocCall = ctx->irCtx->builder.CreateCall(
			    mallocFn,
			    {llvm::ConstantInt::get(mallocFn->getArg(0)->getType(),
			                            (usize)ctx->irCtx->dataLayout.getTypeAllocSize(exprTy->get_llvm_type()))});
			result = ir::Value::get(
			    mallocCall, ir::MarkType::get(true, exprTy, false, ir::MarkOwner::of_heap(), false, ctx->irCtx), false);
		} else if (is_target_region()) {
			auto regRes = target_as_region()->emit(ctx);
			if (not regRes->is_region()) {
				ctx->Error("Expected this type to be a region as suggested by the in-expression, but got the type " +
				               ctx->color(regRes->to_string()) + " instead",
				           target_as_region()->fileRange);
			}
			result = regRes->as_region()->ownData(exprTy, None, ctx->irCtx);
		} else { // TODO - Support other allocators
			ctx->Error("Unsupported type of allocator for in-expression", fileRange);
		}
		dummyValue->get_llvm()->replaceAllUsesWith(result->get_llvm());
		auto ptrTy = llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace());
		ctx->irCtx->builder.CreateCondBr(
		    ctx->irCtx->builder.CreateICmpNE(
		        ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx), result->get_llvm(),
		                                          llvm::ConstantPointerNull::get(ptrTy)),
		        llvm::ConstantInt::get(
		            llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getPointerTypeSizeInBits(ptrTy)),
		            0u)),
		    nonNullBlock->get_bb(), restBlock->get_bb());
		currBlock->set_active(ctx->irCtx->builder);
	} else {
		auto resExp = candidate->emit(ctx);
		auto expTy  = resExp->get_pass_type();
		if (is_type_inferred()) {
			if (not expTy->is_same(finalTy->get_subtype())) {
				ctx->Error("Expected an expression of type " + ctx->color(finalTy->get_subtype()->to_string()) +
				               " here, but the type of the expression obtained is " +
				               ctx->color(resExp->get_ir_type()->to_string()),
				           candidate->fileRange);
			}
		}
		auto finalVal = ir::Logic::handle_pass_semantics(ctx, expTy, resExp, candidate->fileRange);

		auto currBlock = ctx->get_fn()->get_block();
		startBlock->set_active(ctx->irCtx->builder);
		if (is_target_heap()) {
			auto mallocName = ctx->mod->link_internal_dependency(ir::InternalDependency::malloc, ctx->irCtx, fileRange);
			auto mallocFn   = ctx->mod->get_llvm_module()->getFunction(mallocName);
			auto mallocCall = ctx->irCtx->builder.CreateCall(
			    mallocFn,
			    {llvm::ConstantInt::get(mallocFn->getArg(0)->getType(),
			                            (usize)ctx->irCtx->dataLayout.getTypeAllocSize(expTy->get_llvm_type()))});
			result = ir::Value::get(
			    mallocCall, ir::MarkType::get(true, expTy, false, ir::MarkOwner::of_heap(), false, ctx->irCtx), false);
		} else if (is_target_region()) {
			auto regRes = target_as_region()->emit(ctx);
			if (not regRes->is_region()) {
				ctx->Error("Expected this type to be a region as suggested by the in-expression, but got the type " +
				               ctx->color(regRes->to_string()) + " instead",
				           target_as_region()->fileRange);
			}
			result = regRes->as_region()->ownData(expTy, None, ctx->irCtx);
		} else {
			ctx->Error("Unsupported type of allocator for in-expression", fileRange);
		}
		auto ptrTy = llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace());
		ctx->irCtx->builder.CreateCondBr(
		    ctx->irCtx->builder.CreateICmpNE(
		        ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx), result->get_llvm(),
		                                          llvm::ConstantPointerNull::get(ptrTy)),
		        llvm::ConstantInt::get(
		            llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getPointerTypeSizeInBits(ptrTy)),
		            0u)),
		    nonNullBlock->get_bb(), restBlock->get_bb());
		currBlock->set_active(ctx->irCtx->builder);
		ctx->irCtx->builder.CreateStore(finalVal->get_llvm(), result->get_llvm());
	}
	ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
	restBlock->set_active(ctx->irCtx->builder);
	return result->with_range(fileRange);
}

} // namespace qat::ast
