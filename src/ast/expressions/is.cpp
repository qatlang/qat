#include "./is.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IR::Value* IsExpression::emit(IR::Context* ctx) {
  if (subExpr) {
    SHOW("Found sub expression")
    if (isLocalDecl() && !localValue->getType()->isMaybe()) {
      ctx->Error("Expected an expression of type " + ctx->highlightError(localValue->getType()->toString()) +
                     ", but found an is expression",
                 fileRange);
    } else if (isLocalDecl() && !localValue->getType()->asMaybe()->hasSizedSubType(ctx)) {
      auto* subIR = subExpr->emit(ctx);
      if (localValue->getType()->asMaybe()->getSubType()->isSame(subIR->getType()) ||
          (subIR->getType()->isReference() &&
           localValue->getType()->asMaybe()->getSubType()->isSame(subIR->getType()->asReference()->getSubType()))) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                 localValue->getLLVM());
        return nullptr;
      } else {
        ctx->Error("Expected an expression of type " +
                       ctx->highlightError(localValue->getType()->asMaybe()->getSubType()->toString()) +
                       " for the declaration, but found an expression of type " +
                       ctx->highlightError(subIR->getType()->toString()),
                   fileRange);
      }
    }
    if (isTypeInferred()) {
      if (inferredType->isMaybe()) {
        if (subExpr->hasTypeInferrance()) {
          subExpr->asTypeInferrable()->setInferenceType(inferredType->asMaybe()->getSubType());
        }
      } else {
        ctx->Error("Expected type is " + ctx->highlightError(inferredType->toString()) +
                       ", but an `is` expression is provided",
                   fileRange);
      }
    }
    auto* subIR   = subExpr->emit(ctx);
    auto* subType = subIR->getType();
    auto* expectSubTy =
        isLocalDecl()
            ? localValue->getType()->asMaybe()->getSubType()
            : (confirmedRef
                   ? (subType->isReference() ? subType->asReference() : IR::ReferenceType::get(isRefVar, subType, ctx))
                   : (subType->isReference() ? subType->asReference()->getSubType() : subType));
    if (isLocalDecl() && localValue->getType()->isMaybe() &&
        !localValue->getType()->asMaybe()->getSubType()->isSame(subType) &&
        !(subType->isReference() &&
          localValue->getType()->asMaybe()->getSubType()->isSame(subType->asReference()->getSubType()))) {
      ctx->Error("Expected an expression of type " +
                     ctx->highlightError(localValue->getType()->asMaybe()->getSubType()->toString()) +
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
        if (subType->asExpanded()->hasCopyConstructor()) {
          mFn = subType->asExpanded()->getCopyConstructor();
        } else if (subType->asExpanded()->hasMoveConstructor()) {
          mFn = subType->asExpanded()->getMoveConstructor();
        }
      }
      if (mFn != nullptr) {
        llvm::Value* maybeTagPtr   = nullptr;
        llvm::Value* maybeValuePtr = nullptr;
        IR::Value*   returnValue   = nullptr;
        if (isLocalDecl()) {
          maybeValuePtr =
              ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 1u);
          maybeTagPtr = ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 0u);
        } else {
          auto* maybeTy = IR::MaybeType::get(subType, false, ctx);
          auto* block   = ctx->getActiveFunction()->getBlock();
          auto* loc     = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
                                      irName.has_value() ? irName->range : fileRange);
          maybeTagPtr   = ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), loc->getAlloca(), 0u);
          maybeValuePtr = ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), loc->getAlloca(), 1u);
          returnValue   = loc->toNewIRValue();
        }
        (void)mFn->call(ctx, {maybeValuePtr, subIR->getLLVM()}, None, ctx->getMod());
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false), maybeTagPtr);
        return returnValue;
      } else {
        auto* subValue = ctx->builder.CreateLoad(expectSubTy->getLLVMType(), subIR->getLLVM());
        auto* maybeTy  = IR::MaybeType::get(expectSubTy, false, ctx);
        if (isLocalDecl()) {
          ctx->builder.CreateStore(subValue,
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), localValue->getAlloca(), 1u));
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   ctx->builder.CreateStructGEP(maybeTy->getLLVMType(), localValue->getAlloca(), 0u));
          return nullptr;
        } else {
          auto* block  = ctx->getActiveFunction()->getBlock();
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
        if (isLocalDecl()) {
          if (expectSubTy->asReference()->getSubType()->isTypeSized()) {
            ctx->builder.CreateStore(
                ctx->builder.CreatePointerCast(
                    subIR->getLLVM(), llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout->getProgramAddressSpace())),
                ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 1u));
          } else {
            ctx->builder.CreateStore(
                subIR->getLLVM(),
                ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 1u));
          }
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 0u));
          return nullptr;
        } else {
          auto maybeTy  = IR::MaybeType::get(expectSubTy, false, ctx);
          auto newValue = ctx->getActiveFunction()->getBlock()->newValue(
              irName.has_value() ? irName->value : utils::unique_id(), maybeTy, isVar,
              irName.has_value() ? irName->range : fileRange);
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
      if (isLocalDecl()) {
        if (expectSubTy->isTypeSized()) {
          subIR->loadImplicitPointer(ctx->builder);
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getAlloca(), 0u));
          ctx->builder.CreateStore(subIR->getLLVM(), ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(),
                                                                                  localValue->getAlloca(), 1u));
        } else {
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   localValue->getAlloca());
        }
        return nullptr;
      } else {
        if (expectSubTy->isTypeSized()) {
          auto* newValue = ctx->getActiveFunction()->getBlock()->newValue(
              irName.has_value() ? irName->value : utils::unique_id(), IR::MaybeType::get(expectSubTy, false, ctx),
              irName.has_value() ? isVar : true, fileRange);
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
              ctx->builder.CreateStructGEP(newValue->getType()->getLLVMType(), newValue->getAlloca(), 0u));
          ctx->builder.CreateStore(subIR->getLLVM(), ctx->builder.CreateStructGEP(newValue->getType()->getLLVMType(),
                                                                                  newValue->getAlloca(), 1u));
          return newValue->toNewIRValue();
        } else {
          auto* newValue = ctx->getActiveFunction()->getBlock()->newValue(
              irName.has_value() ? irName->value : utils::unique_id(), IR::MaybeType::get(expectSubTy, false, ctx),
              irName.has_value() ? isVar : true, fileRange);
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   newValue->getAlloca());
          return newValue->toNewIRValue();
        }
      }
    }
  } else {
    if (isLocalDecl()) {
      if (localValue->getType()->isMaybe()) {
        if (localValue->getType()->asMaybe()->hasSizedSubType(ctx)) {
          ctx->Error("Expected an expression of type " +
                         ctx->highlightError(localValue->getType()->asMaybe()->getSubType()->toString()) +
                         ", but no expression was provided",
                     fileRange);
        } else {
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                                   localValue->getAlloca(), false);
          return nullptr;
        }
      } else {
        ctx->Error("Expected expression of type " + ctx->highlightError(localValue->getType()->toString()) +
                       ", but an " + ctx->highlightError("is") + " expression was provided",
                   fileRange);
      }
    } else if (irName.has_value()) {
      auto* resMTy    = IR::MaybeType::get(IR::VoidType::get(ctx->llctx), false, ctx);
      auto* block     = ctx->getActiveFunction()->getBlock();
      auto* newAlloca = block->newValue(irName->value, resMTy, isVar, irName->range);
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                               newAlloca->getAlloca(), false);
      return newAlloca->toNewIRValue();
    } else {
      return new IR::Value(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u, false),
                           IR::MaybeType::get(IR::VoidType::get(ctx->llctx), false, ctx), false, IR::Nature::temporary);
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
