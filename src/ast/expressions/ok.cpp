#include "./ok.hpp"
#include "../../IR/types/result.hpp"

namespace qat::ast {

Ok::Ok(Expression* _subExpr, FileRange _range) : Expression(std::move(_range)), subExpr(_subExpr){};

IR::Value* Ok::emit(IR::Context* ctx) {
  if (isTypeInferred()) {
    if (!inferredType->isResult()) {
      ctx->Error("Inferred type is " + ctx->highlightError(inferredType->toString()) + " cannot be the type of " +
                     ctx->highlightError("ok") + " expression, as it expects a result type",
                 fileRange);
    }
    if (subExpr->hasTypeInferrance()) {
      subExpr->asTypeInferrable()->setInferenceType(inferredType->asResult()->getValidType());
    }
    if (isLocalDecl()) {
      createIn = localValue->toNewIRValue();
    } else if (!canCreateIn()) {
      createIn = ctx->getActiveFunction()->getBlock()->newValue(irName.has_value() ? irName->value : utils::unique_id(),
                                                                inferredType, isVar,
                                                                irName.has_value() ? irName->range : fileRange);
    }
    if (subExpr->isInPlaceCreatable()) {
      auto refTy = IR::ReferenceType::get(createIn->getType()->isReference()
                                              ? createIn->getType()->asReference()->isSubtypeVariable()
                                              : createIn->isVariable(),
                                          inferredType->asResult()->getValidType(), ctx);
      subExpr->asInPlaceCreatable()->setCreateIn(new IR::Value(
          ctx->builder.CreatePointerCast(
              ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 1u), refTy->getLLVMType()),
          refTy, false, IR::Nature::temporary));
    }
  } else {
    ctx->Error(ctx->highlightError("ok") + " expression expects the type to be inferred from scope", fileRange);
  }
  auto* expr = subExpr->emit(ctx);
  SHOW("Sub expression emitted")
  if (subExpr->isInPlaceCreatable()) {
    ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                             ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 0u));
    return createIn;
  }
  if (isTypeInferred() && !(expr->getType()->isSame(inferredType->asResult()->getValidType()) ||
                            (expr->getType()->isReference() && expr->getType()->asReference()->getSubType()->isSame(
                                                                   inferredType->asResult()->getValidType())))) {
    ctx->Error("Provided expression is of incompatible type " + ctx->highlightError(expr->getType()->toString()) +
                   ". Expected an expression of type " +
                   ctx->highlightError(inferredType->asResult()->getValidType()->toString()),
               subExpr->fileRange);
  }
  if ((expr->getType()->isSame(inferredType->asResult()->getValidType()) && expr->isImplicitPointer()) ||
      expr->getType()->isReference()) {
    if (!expr->getType()->isSame(inferredType->asResult()->getValidType())) {
      expr->loadImplicitPointer(ctx->builder);
    }
    auto validTy = inferredType->asResult()->getValidType();
    if (validTy->isTriviallyCopyable() || validTy->isTriviallyMovable()) {
      ctx->builder.CreateStore(
          ctx->builder.CreateLoad(validTy->getLLVMType(), expr->getLLVM()),
          ctx->builder.CreatePointerCast(
              ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 1u),
              llvm::PointerType::get(validTy->getLLVMType(), ctx->dataLayout->getProgramAddressSpace())));
      ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 0u));
      if (!validTy->isTriviallyCopyable()) {
        // MOVE WARNING
        ctx->Warning("There is a trivial move occuring here. Do you want to use " + ctx->highlightWarning("'move") +
                         " to make it explicit and clear?",
                     subExpr->fileRange);
        ctx->builder.CreateStore(llvm::ConstantExpr::getNullValue(validTy->getLLVMType()), expr->getLLVM());
        if (expr->isLocalToFn()) {
          ctx->getActiveFunction()->getBlock()->addMovedValue(expr->getLocalID());
        }
      }
      return createIn;
    } else {
      // NON-TRIVIAL COPY & MOVE ERROR
      ctx->Error("The expression provided is of type " + ctx->highlightError(expr->getType()->toString()) + ". Type " +
                     ctx->highlightError(validTy->toString()) + " cannot be trivially copied or moved. Please do " +
                     ctx->highlightError("'copy") + " or " + ctx->highlightError("'move"),
                 subExpr->fileRange);
    }
  } else if (expr->getType()->isSame(inferredType->asResult()->getValidType())) {
    auto validTy = inferredType->asResult()->getValidType();
    ctx->builder.CreateStore(
        expr->getLLVM(),
        ctx->builder.CreatePointerCast(
            ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 1u),
            llvm::PointerType::get(validTy->getLLVMType(), ctx->dataLayout->getProgramAddressSpace())));
    ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                             ctx->builder.CreateStructGEP(inferredType->getLLVMType(), createIn->getLLVM(), 0u));
  }
  return createIn;
}

Json Ok::toJson() const {
  return Json()._("nodeType", "ok")._("hasSubExpression", subExpr != nullptr)._("fileRange", fileRange);
}

} // namespace qat::ast