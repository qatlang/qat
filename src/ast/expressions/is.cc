#include "./is.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::Value* IsExpression::emit(EmitCtx* ctx) {
	if (subExpr) {
		SHOW("Found sub expression")
		if (isLocalDecl() && not localValue->get_ir_type()->is_maybe()) {
			ctx->Error("Expected an expression of type " + ctx->color(localValue->get_ir_type()->to_string()) +
			               ", but found an is expression",
			           fileRange);
		} else if (isLocalDecl() && not localValue->get_ir_type()->as_maybe()->has_sized_sub_type(ctx->irCtx)) {
			auto* subIR = subExpr->emit(ctx);
			if (localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subIR->get_ir_type()) ||
			    (subIR->get_ir_type()->is_ref() && localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(
			                                           subIR->get_ir_type()->as_ref()->get_subtype()))) {
				ctx->irCtx->builder.CreateStore(
				    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
				    localValue->get_llvm());
				return nullptr;
			} else {
				ctx->Error("Expected an expression of type " +
				               ctx->color(localValue->get_ir_type()->as_maybe()->get_subtype()->to_string()) +
				               " for the declaration, but found an expression of type " +
				               ctx->color(subIR->get_ir_type()->to_string()),
				           fileRange);
			}
		}
		if (is_type_inferred()) {
			if (inferredType->is_maybe()) {
				if (subExpr->has_type_inferrance()) {
					subExpr->as_type_inferrable()->set_inference_type(inferredType->as_maybe()->get_subtype());
				}
			} else {
				ctx->Error("Expected type is " + ctx->color(inferredType->to_string()) +
				               ", but an `is` expression is provided",
				           fileRange);
			}
		}
		auto* subIR       = subExpr->emit(ctx);
		auto* subType     = subIR->get_ir_type();
		auto* expectSubTy = isLocalDecl()
		                        ? localValue->get_ir_type()->as_maybe()->get_subtype()
		                        : (confirmedRef ? (subType->is_ref() ? subType->as_ref()
		                                                             : ir::RefType::get(isRefVar, subType, ctx->irCtx))
		                                        : (subType->is_ref() ? subType->as_ref()->get_subtype() : subType));
		if (isLocalDecl() && localValue->get_ir_type()->is_maybe() &&
		    not localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subType) &&
		    not(subType->is_ref() &&
		        localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subType->as_ref()->get_subtype()))) {
			ctx->Error("Expected an expression of type " +
			               ctx->color(localValue->get_ir_type()->as_maybe()->get_subtype()->to_string()) +
			               ", but found an expression of type " + ctx->color(subType->to_string()),
			           fileRange);
		}
		if ((subType->is_ref() || subIR->is_ghost_ref()) && not expectSubTy->is_ref()) {
			if (subType->is_ref()) {
				subType = subType->as_ref()->get_subtype();
				subIR->load_ghost_ref(ctx->irCtx->builder);
			}
			ir::Method* mFn = nullptr;
			if (subType->is_expanded()) {
				if (subType->as_expanded()->has_copy_constructor()) {
					mFn = subType->as_expanded()->get_copy_constructor();
				} else if (subType->as_expanded()->has_move_constructor()) {
					mFn = subType->as_expanded()->get_move_constructor();
				}
			}
			if (mFn != nullptr) {
				llvm::Value* maybeTagPtr   = nullptr;
				llvm::Value* maybeValuePtr = nullptr;
				ir::Value*   returnValue   = nullptr;
				if (isLocalDecl()) {
					maybeValuePtr = ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
					                                                    localValue->get_alloca(), 1u);
					maybeTagPtr   = ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
					                                                    localValue->get_alloca(), 0u);
				} else {
					auto* maybeTy = ir::MaybeType::get(subType, false, ctx->irCtx);
					auto* block   = ctx->get_fn()->get_block();
					auto* loc     = block->new_local(irName.has_value() ? irName->value : utils::uid_string(), maybeTy,
					                             isVar, irName.has_value() ? irName->range : fileRange);
					maybeTagPtr = ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), loc->get_alloca(), 0u);
					maybeValuePtr =
					    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), loc->get_alloca(), 1u);
					returnValue = loc->to_new_ir_value();
				}
				(void)mFn->call(ctx->irCtx, {maybeValuePtr, subIR->get_llvm()}, None, ctx->mod);
				ctx->irCtx->builder.CreateStore(
				    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false), maybeTagPtr);
				return returnValue;
			} else {
				auto* subValue = ctx->irCtx->builder.CreateLoad(expectSubTy->get_llvm_type(), subIR->get_llvm());
				auto* maybeTy  = ir::MaybeType::get(expectSubTy, false, ctx->irCtx);
				if (isLocalDecl()) {
					ctx->irCtx->builder.CreateStore(
					    subValue,
					    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), localValue->get_alloca(), 1u));
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), localValue->get_alloca(), 0u));
					return nullptr;
				} else {
					auto* block  = ctx->get_fn()->get_block();
					auto* newLoc = block->new_local(irName.has_value() ? irName->value : utils::uid_string(), maybeTy,
					                                isVar, irName.has_value() ? irName->range : fileRange);
					ctx->irCtx->builder.CreateStore(subValue, ctx->irCtx->builder.CreateStructGEP(
					                                              maybeTy->get_llvm_type(), newLoc->get_alloca(), 1u));
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), newLoc->get_alloca(), 0u));
					return newLoc->to_new_ir_value();
				}
			}
		} else if (expectSubTy->is_ref()) {
			if (subType->is_ref() || subIR->is_ghost_ref()) {
				if (subType->is_ref()) {
					subIR->load_ghost_ref(ctx->irCtx->builder);
				}
				if (isLocalDecl()) {
					if (expectSubTy->as_ref()->get_subtype()->is_type_sized()) {
						ctx->irCtx->builder.CreateStore(
						    ctx->irCtx->builder.CreatePointerCast(
						        subIR->get_llvm(), llvm::Type::getInt8Ty(ctx->irCtx->llctx)
						                               ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace())),
						    ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
						                                        localValue->get_alloca(), 1u));
					} else {
						ctx->irCtx->builder.CreateStore(
						    subIR->get_llvm(),
						    ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
						                                        localValue->get_alloca(), 1u));
					}
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
					                                        localValue->get_alloca(), 0u));
					return nullptr;
				} else {
					auto maybeTy   = ir::MaybeType::get(expectSubTy, false, ctx->irCtx);
					auto new_value = ctx->get_fn()->get_block()->new_local(
					    irName.has_value() ? irName->value : utils::uid_string(), maybeTy, isVar,
					    irName.has_value() ? irName->range : fileRange);
					if (expectSubTy->as_ref()->get_subtype()->is_type_sized()) {
						ctx->irCtx->builder.CreateStore(
						    ctx->irCtx->builder.CreatePointerCast(
						        subIR->get_llvm(), llvm::Type::getInt8Ty(ctx->irCtx->llctx)
						                               ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace())),
						    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), new_value->get_alloca(), 1u));
					} else {
						ctx->irCtx->builder.CreateStore(
						    subIR->get_llvm(),
						    ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), new_value->get_alloca(), 1u));
					}
					return new_value->to_new_ir_value();
				}
			} else {
				ctx->Error("Expected a reference, found a value of type " + ctx->color(subType->to_string()),
				           subExpr->fileRange);
			}
		} else {
			if (isLocalDecl()) {
				if (expectSubTy->is_type_sized()) {
					subIR->load_ghost_ref(ctx->irCtx->builder);
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
					                                        localValue->get_alloca(), 0u));
					ctx->irCtx->builder.CreateStore(subIR->get_llvm(), ctx->irCtx->builder.CreateStructGEP(
					                                                       localValue->get_ir_type()->get_llvm_type(),
					                                                       localValue->get_alloca(), 1u));
				} else {
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    localValue->get_alloca());
				}
				return nullptr;
			} else {
				if (expectSubTy->is_type_sized()) {
					auto* new_value =
					    ctx->get_fn()->get_block()->new_local(irName.has_value() ? irName->value : utils::uid_string(),
					                                          ir::MaybeType::get(expectSubTy, false, ctx->irCtx),
					                                          irName.has_value() ? isVar : true, fileRange);
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    ctx->irCtx->builder.CreateStructGEP(new_value->get_ir_type()->get_llvm_type(),
					                                        new_value->get_alloca(), 0u));
					ctx->irCtx->builder.CreateStore(
					    subIR->get_llvm(), ctx->irCtx->builder.CreateStructGEP(
					                           new_value->get_ir_type()->get_llvm_type(), new_value->get_alloca(), 1u));
					return new_value->to_new_ir_value();
				} else {
					auto* new_value =
					    ctx->get_fn()->get_block()->new_local(irName.has_value() ? irName->value : utils::uid_string(),
					                                          ir::MaybeType::get(expectSubTy, false, ctx->irCtx),
					                                          irName.has_value() ? isVar : true, fileRange);
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    new_value->get_alloca());
					return new_value->to_new_ir_value();
				}
			}
		}
	} else {
		if (isLocalDecl()) {
			if (localValue->get_ir_type()->is_maybe()) {
				if (localValue->get_ir_type()->as_maybe()->has_sized_sub_type(ctx->irCtx)) {
					ctx->Error("Expected an expression of type " +
					               ctx->color(localValue->get_ir_type()->as_maybe()->get_subtype()->to_string()) +
					               ", but no expression was provided",
					           fileRange);
				} else {
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
					    localValue->get_alloca(), false);
					return nullptr;
				}
			} else {
				ctx->Error("Expected expression of type " + ctx->color(localValue->get_ir_type()->to_string()) +
				               ", but an " + ctx->color("is") + " expression was provided",
				           fileRange);
			}
		} else if (irName.has_value()) {
			auto* resMTy    = ir::MaybeType::get(ir::VoidType::get(ctx->irCtx->llctx), false, ctx->irCtx);
			auto* block     = ctx->get_fn()->get_block();
			auto* newAlloca = block->new_local(irName->value, resMTy, isVar, irName->range);
			ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
			                                newAlloca->get_alloca(), false);
			return newAlloca->to_new_ir_value();
		} else {
			return ir::Value::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
			                      ir::MaybeType::get(ir::VoidType::get(ctx->irCtx->llctx), false, ctx->irCtx), false);
		}
	}
}

Json IsExpression::to_json() const {
	return Json()
	    ._("nodeType", "isExpression")
	    ._("hasSubExpression", subExpr != nullptr)
	    ._("subExpression", subExpr ? subExpr->to_json() : JsonValue())
	    ._("isRef", confirmedRef)
	    ._("isRefVar", isRefVar)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
