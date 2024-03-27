#include "./is.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ir::Value* IsExpression::emit(EmitCtx* ctx) {
  if (subExpr) {
    SHOW("Found sub expression")
    if (isLocalDecl() && !localValue->get_ir_type()->is_maybe()) {
      ctx->Error("Expected an expression of type " + ctx->color(localValue->get_ir_type()->to_string()) +
                     ", but found an is expression",
                 fileRange);
    } else if (isLocalDecl() && !localValue->get_ir_type()->as_maybe()->has_sized_sub_type(ctx->irCtx)) {
      auto* subIR = subExpr->emit(ctx);
      if (localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subIR->get_ir_type()) ||
          (subIR->get_ir_type()->is_reference() && localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(
                                                       subIR->get_ir_type()->as_reference()->get_subtype()))) {
        ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
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
        ctx->Error("Expected type is " + ctx->color(inferredType->to_string()) + ", but an `is` expression is provided",
                   fileRange);
      }
    }
    auto* subIR   = subExpr->emit(ctx);
    auto* subType = subIR->get_ir_type();
    auto* expectSubTy =
        isLocalDecl()
            ? localValue->get_ir_type()->as_maybe()->get_subtype()
            : (confirmedRef ? (subType->is_reference() ? subType->as_reference()
                                                       : ir::ReferenceType::get(isRefVar, subType, ctx->irCtx))
                            : (subType->is_reference() ? subType->as_reference()->get_subtype() : subType));
    if (isLocalDecl() && localValue->get_ir_type()->is_maybe() &&
        !localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subType) &&
        !(subType->is_reference() &&
          localValue->get_ir_type()->as_maybe()->get_subtype()->is_same(subType->as_reference()->get_subtype()))) {
      ctx->Error("Expected an expression of type " +
                     ctx->color(localValue->get_ir_type()->as_maybe()->get_subtype()->to_string()) +
                     ", but found an expression of type " + ctx->color(subType->to_string()),
                 fileRange);
    }
    if ((subType->is_reference() || subIR->is_ghost_pointer()) && !expectSubTy->is_reference()) {
      if (subType->is_reference()) {
        subType = subType->as_reference()->get_subtype();
        subIR->load_ghost_pointer(ctx->irCtx->builder);
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
                                                              localValue->getAlloca(), 1u);
          maybeTagPtr   = ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
                                                              localValue->getAlloca(), 0u);
        } else {
          auto* maybeTy = ir::MaybeType::get(subType, false, ctx->irCtx);
          auto* block   = ctx->get_fn()->get_block();
          auto* loc     = block->new_value(irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
                                       irName.has_value() ? irName->range : fileRange);
          maybeTagPtr   = ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), loc->getAlloca(), 0u);
          maybeValuePtr = ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), loc->getAlloca(), 1u);
          returnValue   = loc->to_new_ir_value();
        }
        (void)mFn->call(ctx->irCtx, {maybeValuePtr, subIR->get_llvm()}, None, ctx->mod);
        ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                        maybeTagPtr);
        return returnValue;
      } else {
        auto* subValue = ctx->irCtx->builder.CreateLoad(expectSubTy->get_llvm_type(), subIR->get_llvm());
        auto* maybeTy  = ir::MaybeType::get(expectSubTy, false, ctx->irCtx);
        if (isLocalDecl()) {
          ctx->irCtx->builder.CreateStore(
              subValue, ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), localValue->getAlloca(), 1u));
          ctx->irCtx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
              ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), localValue->getAlloca(), 0u));
          return nullptr;
        } else {
          auto* block  = ctx->get_fn()->get_block();
          auto* newLoc = block->new_value(irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
                                          irName.has_value() ? irName->range : fileRange);
          ctx->irCtx->builder.CreateStore(
              subValue, ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), newLoc->getAlloca(), 1u));
          ctx->irCtx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
              ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), newLoc->getAlloca(), 0u));
          return newLoc->to_new_ir_value();
        }
      }
    } else if (expectSubTy->is_reference()) {
      if (subType->is_reference() || subIR->is_ghost_pointer()) {
        if (subType->is_reference()) {
          subIR->load_ghost_pointer(ctx->irCtx->builder);
        }
        if (isLocalDecl()) {
          if (expectSubTy->as_reference()->get_subtype()->is_type_sized()) {
            ctx->irCtx->builder.CreateStore(
                ctx->irCtx->builder.CreatePointerCast(
                    subIR->get_llvm(),
                    llvm::Type::getInt8PtrTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout->getProgramAddressSpace())),
                ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(), localValue->getAlloca(),
                                                    1u));
          } else {
            ctx->irCtx->builder.CreateStore(
                subIR->get_llvm(), ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
                                                                       localValue->getAlloca(), 1u));
          }
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          ctx->irCtx->builder.CreateStructGEP(
                                              localValue->get_ir_type()->get_llvm_type(), localValue->getAlloca(), 0u));
          return nullptr;
        } else {
          auto maybeTy = ir::MaybeType::get(expectSubTy, false, ctx->irCtx);
          auto new_value =
              ctx->get_fn()->get_block()->new_value(irName.has_value() ? irName->value : utils::unique_id(), maybeTy,
                                                    isVar, irName.has_value() ? irName->range : fileRange);
          if (expectSubTy->as_reference()->get_subtype()->is_type_sized()) {
            ctx->irCtx->builder.CreateStore(
                ctx->irCtx->builder.CreatePointerCast(
                    subIR->get_llvm(),
                    llvm::Type::getInt8PtrTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout->getProgramAddressSpace())),
                ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), new_value->getAlloca(), 1u));
          } else {
            ctx->irCtx->builder.CreateStore(
                subIR->get_llvm(),
                ctx->irCtx->builder.CreateStructGEP(maybeTy->get_llvm_type(), new_value->getAlloca(), 1u));
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
          subIR->load_ghost_pointer(ctx->irCtx->builder);
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          ctx->irCtx->builder.CreateStructGEP(
                                              localValue->get_ir_type()->get_llvm_type(), localValue->getAlloca(), 0u));
          ctx->irCtx->builder.CreateStore(
              subIR->get_llvm(), ctx->irCtx->builder.CreateStructGEP(localValue->get_ir_type()->get_llvm_type(),
                                                                     localValue->getAlloca(), 1u));
        } else {
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          localValue->getAlloca());
        }
        return nullptr;
      } else {
        if (expectSubTy->is_type_sized()) {
          auto* new_value = ctx->get_fn()->get_block()->new_value(
              irName.has_value() ? irName->value : utils::unique_id(),
              ir::MaybeType::get(expectSubTy, false, ctx->irCtx), irName.has_value() ? isVar : true, fileRange);
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          ctx->irCtx->builder.CreateStructGEP(new_value->get_ir_type()->get_llvm_type(),
                                                                              new_value->getAlloca(), 0u));
          ctx->irCtx->builder.CreateStore(subIR->get_llvm(),
                                          ctx->irCtx->builder.CreateStructGEP(new_value->get_ir_type()->get_llvm_type(),
                                                                              new_value->getAlloca(), 1u));
          return new_value->to_new_ir_value();
        } else {
          auto* new_value = ctx->get_fn()->get_block()->new_value(
              irName.has_value() ? irName->value : utils::unique_id(),
              ir::MaybeType::get(expectSubTy, false, ctx->irCtx), irName.has_value() ? isVar : true, fileRange);
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          new_value->getAlloca());
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
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                          localValue->getAlloca(), false);
          return nullptr;
        }
      } else {
        ctx->Error("Expected expression of type " + ctx->color(localValue->get_ir_type()->to_string()) + ", but an " +
                       ctx->color("is") + " expression was provided",
                   fileRange);
      }
    } else if (irName.has_value()) {
      auto* resMTy    = ir::MaybeType::get(ir::VoidType::get(ctx->irCtx->llctx), false, ctx->irCtx);
      auto* block     = ctx->get_fn()->get_block();
      auto* newAlloca = block->new_value(irName->value, resMTy, isVar, irName->range);
      ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u, false),
                                      newAlloca->getAlloca(), false);
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
