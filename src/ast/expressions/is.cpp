#include "./is.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IsExpression::IsExpression(Expression* _subExpr, FileRange _fileRange)
    : Expression(std::move(_fileRange)), subExpr(_subExpr) {}

IR::Value* IsExpression::emit(IR::Context* ctx) {
  if (subExpr) {
    if (local && !local->getType()->isMaybe()) {
      ctx->Error("Expected an expression of type " + ctx->highlightError(local->getType()->toString()) +
                     ", but found an is expression",
                 fileRange);
    } else if (local && !local->getType()->asMaybe()->hasSizedSubType(ctx)) {
      auto* subIR = subExpr->emit(ctx);
      if (local->getType()->asMaybe()->getSubType()->isSame(subIR->getType()) ||
          (subIR->getType()->isReference() &&
           local->getType()->asMaybe()->getSubType()->isSame(subIR->getType()->asReference()->getSubType()))) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                 local->getLLVM());
        return nullptr;
      } else {
        ctx->Error("Expected an expression of type " +
                       ctx->highlightError(local->getType()->asMaybe()->getSubType()->toString()) +
                       " for the declaration, but found an expression of type " +
                       ctx->highlightError(subIR->getType()->toString()),
                   fileRange);
      }
    }
    if (inferredType) {
      if (inferredType.value()->isMaybe()) {
        subExpr->setInferenceType(inferredType.value()->asMaybe()->getSubType());
      } else {
        ctx->Error("Expected type is " + ctx->highlightError(inferredType.value()->toString()) +
                       ", but an `is` expression is provided",
                   fileRange);
      }
    }
    auto* subIR   = subExpr->emit(ctx);
    auto* subType = subIR->getType();
    auto* expectSubTy =
        local
            ? local->getType()->asMaybe()->getSubType()
            : (confirmedRef
                   ? (subType->isReference() ? subType->asReference() : IR::ReferenceType::get(isRefVar, subType, ctx))
                   : (subType->isReference() ? subType->asReference()->getSubType() : subType));
    if (local && local->getType()->isMaybe() && !local->getType()->asMaybe()->getSubType()->isSame(subType) &&
        !(subType->isReference() &&
          local->getType()->asMaybe()->getSubType()->isSame(subType->asReference()->getSubType()))) {
      ctx->Error("Expected an expression of type " +
                     ctx->highlightError(local->getType()->asMaybe()->getSubType()->toString()) +
                     ", but found an expression of type " + ctx->highlightError(subType->toString()),
                 fileRange);
    }
    if ((subType->isReference() || subIR->isImplicitPointer()) && !expectSubTy->isReference()) {
      if (subType->isReference()) {
        subType = subType->asReference()->getSubType();
        subIR->loadImplicitPointer(ctx->builder);
      }
      IR::MemberFunction* mFn = nullptr;
      if (subType->isExpanded()) {
        if (subType->asExpanded()->hasCopy()) {
          mFn = subType->asExpanded()->getCopyConstructor();
        } else if (subType->asExpanded()->hasMove()) {
          mFn = subType->asExpanded()->getMoveConstructor();
        }
      }
      if (mFn != nullptr) {
        llvm::Value* maybeTagPtr   = nullptr;
        llvm::Value* maybeValuePtr = nullptr;
        IR::Value*   returnValue   = nullptr;
        if (local) {
          maybeValuePtr = ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u);
          maybeTagPtr   = ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u);
        } else {
          auto* maybeTy = IR::MaybeType::get(subType, ctx);
          auto* block   = ctx->fn->getBlock();
          auto* loc     = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
                                      irName.has_value() ? irName->range : fileRange);
          maybeTagPtr   = ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), loc->getAlloca(), 0u);
          maybeValuePtr = ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), loc->getAlloca(), 1u);
          returnValue   = loc->toNewIRValue();
        }
        (void)mFn->call(ctx, {maybeValuePtr, subIR->getLLVM()}, ctx->getMod());
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false), maybeTagPtr);
        return returnValue;
      } else {
        auto* subValue = ctx->builder.CreateLoad(expectSubTy->getLLVMType(), subIR->getLLVM());
        auto* maybeTy  = IR::MaybeType::get(expectSubTy, ctx);
        if (local) {
          ctx->builder.CreateStore(subValue,
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), local->getAlloca(), 1u));
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), local->getAlloca(), 0u));
          return nullptr;
        } else {
          auto* block  = ctx->fn->getBlock();
          auto* newLoc = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
                                         irName.has_value() ? irName->range : fileRange);
          ctx->builder.CreateStore(subValue,
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), newLoc->getAlloca(), 1u));
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), newLoc->getAlloca(), 0u));
          return newLoc->toNewIRValue();
        }
      }
    } else if (expectSubTy->isReference()) {
      if (subType->isReference() || subIR->isImplicitPointer()) {
        if (subType->isReference()) {
          subIR->loadImplicitPointer(ctx->builder);
        }
        if (local) {
          if (expectSubTy->asReference()->getSubType()->isTypeSized()) {
            ctx->builder.CreateStore(
                ctx->builder.CreatePointerCast(
                    subIR->getLLVM(), llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout->getProgramAddressSpace())),
                ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u));
          } else {
            ctx->builder.CreateStore(subIR->getLLVM(), ctx->builder.CreateStructGEP(local->getType()->getLLVMType(),
                                                                                    local->getAlloca(), 1u));
          }
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u));
          return nullptr;
        } else {
          auto maybeTy  = IR::MaybeType::get(expectSubTy, ctx);
          auto newValue = ctx->fn->getBlock()->newValue(irName.has_value() ? irName->value : utils::unique_id(),
                                                        maybeTy, isVar, irName.has_value() ? irName->range : fileRange);
          if (expectSubTy->asReference()->getSubType()->isTypeSized()) {
            ctx->builder.CreateStore(
                ctx->builder.CreatePointerCast(
                    subIR->getLLVM(), llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout->getProgramAddressSpace())),
                ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), newValue->getAlloca(), 1u));
          } else {
            ctx->builder.CreateStore(subIR->getLLVM(),
                                     ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), newValue->getAlloca(), 1u));
          }
          return newValue->toNewIRValue();
        }
      } else {
        ctx->Error("Expected a reference, found a value of type " + ctx->highlightError(subType->toString()),
                   subExpr->fileRange);
      }
    } else {
      if (local) {
        if (expectSubTy->isTypeSized()) {
          subIR->loadImplicitPointer(ctx->builder);
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u));
          ctx->builder.CreateStore(
              subIR->getLLVM(), ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u));
        } else {
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   local->getAlloca());
        }
        return nullptr;
      } else {
        if (expectSubTy->isTypeSized()) {
          auto* newValue = ctx->fn->getBlock()->newValue(irName.has_value() ? irName->value : utils::unique_id(),
                                                         IR::MaybeType::get(expectSubTy, ctx),
                                                         irName.has_value() ? isVar : true, fileRange);
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(newValue->getType()->getLLVMType(), newValue->getAlloca(), 0u));
          ctx->builder.CreateStore(subIR->getLLVM(), ctx->builder.CreateStructGEP(newValue->getType()->getLLVMType(),
                                                                                  newValue->getAlloca(), 1u));
          return newValue->toNewIRValue();
        } else {
          auto* newValue = ctx->fn->getBlock()->newValue(irName.has_value() ? irName->value : utils::unique_id(),
                                                         IR::MaybeType::get(expectSubTy, ctx),
                                                         irName.has_value() ? isVar : true, fileRange);
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   newValue->getAlloca());
          return newValue->toNewIRValue();
        }
      }
    }
  } else {
    if (local) {
      if (local->getType()->isMaybe()) {
        if (local->getType()->asMaybe()->hasSizedSubType(ctx)) {
          ctx->Error("Expected an expression of type " +
                         ctx->highlightError(local->getType()->asMaybe()->getSubType()->toString()) +
                         ", but no expression was provided",
                     fileRange);
        } else {
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   local->getAlloca(), false);
          return nullptr;
        }
      } else {
        ctx->Error("Expected expression of type " + ctx->highlightError(local->getType()->toString()) + ", but an " +
                       ctx->highlightError("is") + " expression was provided",
                   fileRange);
      }
    } else if (irName.has_value()) {
      auto* resMTy    = IR::MaybeType::get(IR::VoidType::get(ctx->llctx), ctx);
      auto* block     = ctx->fn->getBlock();
      auto* newAlloca = block->newValue(irName->value, resMTy, isVar, irName->range);
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                               newAlloca->getAlloca(), false);
      return newAlloca->toNewIRValue();
    } else {
      return new IR::Value(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                           IR::MaybeType::get(IR::VoidType::get(ctx->llctx), ctx), false, IR::Nature::temporary);
    }
  }
}

Json IsExpression::toJson() const {
  return Json()
      ._("nodeType", "isExpression")
      ._("hasSubExpression", subExpr != nullptr)
      ._("subExpression", subExpr ? subExpr->toJson() : JsonValue())
      ._("isRef", confirmedRef)
      ._("isRefVar", isRefVar)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
