#include "./is.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::Value* IsExpression::emit(EmitCtx* ctx) {
	if (subExpr) {
		SHOW("Found sub expression")
		if (is_type_inferred()) {
			if (not inferredType->is_maybe()) {
				ctx->Error("Expected type is " + ctx->color(inferredType->to_string()) +
				               ", but an `is` expression is provided",
				           fileRange);
			}
			if (subExpr->has_type_inferrance()) {
				subExpr->as_type_inferrable()->set_inference_type(inferredType->as_maybe()->get_subtype());
			}
		}
		if (canCreateIn()) {
			if (is_type_inferred() && not type_check_create_in(inferredType)) {
				ctx->Error(
				    "Tried to optimise the is-expression by creating in-place. But the type for in-place creation is " +
				        ctx->color(createIn->get_ir_type()->to_string()) + " which is not compatible",
				    fileRange);
			}
		}
		auto boolVal = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u);
		if (subExpr->isPrerunNode() && not canCreateIn()) {
			auto subPre  = subExpr->emit(ctx);
			auto maybeTy = ir::MaybeType::get(subPre->get_ir_type(), false, ctx->irCtx);
			return ir::PrerunValue::get(
			           llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(maybeTy->get_llvm_type()),
			                                     {boolVal, subPre->get_llvm_constant()}),
			           maybeTy)
			    ->with_range(fileRange);
		} else if (subExpr->isInPlaceCreatable()) {
			auto dummyValue = llvm::UndefValue::get(
			    llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace()));
			subExpr->asInPlaceCreatable()->setCreateIn(ir::Value::get(
			    dummyValue, ir::RefType::get(true, ir::IntegerType::get(8u, ctx->irCtx), ctx->irCtx), false));
			auto* subIR   = subExpr->emit(ctx);
			auto* subType = subIR->get_pass_type();
			auto  maybeTy = ir::MaybeType::get(subType, false, ctx->irCtx);
			if (not canCreateIn()) {
				createIn = ctx->get_fn()->get_block()->new_local(
				    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), maybeTy, true,
				    irName.has_value() ? irName->range : fileRange);
			}
			dummyValue->replaceAllUsesWith(
			    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), createIn->get_llvm(), 1u));
			ctx->irCtx->builder.CreateStore(
			    boolVal, ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), createIn->get_llvm(), 0u));
			return get_creation_result(ctx->irCtx, maybeTy, fileRange);
		} else {
			auto* subIR   = subExpr->emit(ctx);
			auto* subType = subIR->get_pass_type();
			auto  maybeTy = ir::MaybeType::get(subType, false, ctx->irCtx);
			if (is_type_inferred() && not inferredType->is_same(maybeTy)) {
				ctx->Error("The type inferred from scope is " + ctx->color(inferredType->to_string()) +
				               ", so this expression was expected to be of type " +
				               ctx->color(inferredType->as_maybe()->get_subtype()->to_string()) +
				               ", but it is of type " + ctx->color(subType->to_string()),
				           subExpr->fileRange);
			}
			if (subIR->is_prerun_value() && not canCreateIn()) {
				return ir::PrerunValue::get(
				           llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(maybeTy->get_llvm_type()),
				                                     {boolVal, subIR->get_llvm_constant()}),
				           maybeTy)
				    ->with_range(fileRange);
			} else {
				if (not canCreateIn()) {
					createIn = ctx->get_fn()->get_block()->new_local(
					    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), maybeTy, true,
					    irName.has_value() ? irName->range : fileRange);
				}
				auto* finalVal = ir::Logic::handle_pass_semantics(ctx, subType, subIR, subExpr->fileRange);
				ctx->irCtx->builder.CreateStore(
				    boolVal, ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), createIn->get_llvm(), 0u));
				ctx->irCtx->builder.CreateStore(
				    finalVal->get_llvm(),
				    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), createIn->get_llvm(), 1u));
				return get_creation_result(ctx->irCtx, maybeTy, fileRange);
			}
		}
	} else {
		auto maybeTy = ir::MaybeType::get(ir::VoidType::get(ctx->irCtx->llctx), false, ctx->irCtx);
		if (isLocalDecl()) {
			createIn = ctx->get_fn()->get_block()->new_local(irName->value, maybeTy, isVar, irName->range);
		}
		if (canCreateIn()) {
			if (not type_check_create_in(maybeTy)) {
				ctx->Error(
				    "Tried to optimise the is-expression by creating in-place, but the type for in-place creation is " +
				        ctx->color(createIn->get_ir_type()->to_string()),
				    fileRange);
			}
			ctx->irCtx->builder.CreateStore(
			    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u),
			    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), createIn->get_llvm(), 0u), false);
			return get_creation_result(ctx->irCtx, maybeTy, fileRange);
		} else {
			return ir::Value::get(llvm::ConstantStruct::get(
			                          llvm::cast<llvm::StructType>(maybeTy->get_llvm_type()),
			                          {llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
			                           llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u)}),
			                      ir::MaybeType::get(ir::VoidType::get(ctx->irCtx->llctx), false, ctx->irCtx), false)
			    ->with_range(fileRange);
		}
	}
}

Json IsExpression::to_json() const {
	return Json()
	    ._("nodeType", "isExpression")
	    ._("hasSubExpression", subExpr != nullptr)
	    ._("subExpression", subExpr ? subExpr->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
