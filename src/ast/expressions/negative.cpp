#include "./negative.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

Negative::Negative(Expression* _value, FileRange _fileRange) : Expression(std::move(_fileRange)), value(_value) {}

IR::Value* Negative::emit(IR::Context* ctx) {
  if (isTypeInferred() && value->hasTypeInferrance()) {
    value->asTypeInferrable()->setInferenceType(inferredType);
  }
  auto irVal     = value->emit(ctx);
  auto valTy     = irVal->getType()->isReference() ? irVal->getType()->asReference()->getSubType() : irVal->getType();
  auto typeCheck = [&](IR::QatType* candTy) {
    if (isTypeInferred()) {
      if (!candTy->isSame(inferredType)) {
        ctx->Error("The expression is of type " + ctx->highlightError(candTy->toString()) +
                       ", but the type inferred from scope is " + ctx->highlightError(inferredType->toString()),
                   value->fileRange);
      }
    }
  };
  if (valTy->isInteger() || (valTy->isCType() && valTy->asCType()->getSubType()->isInteger())) {
    typeCheck(valTy);
    if (irVal->isPrerunValue()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getNeg(irVal->getLLVMConstant()), valTy);
    } else {
      irVal->loadImplicitPointer(ctx->builder);
      if (irVal->getType()->isReference()) {
        irVal = new IR::Value(ctx->builder.CreateLoad(valTy->getLLVMType(), irVal->getLLVM()), valTy, false,
                              IR::Nature::temporary);
      }
      return new IR::Value(ctx->builder.CreateNeg(irVal->getLLVM()), valTy, false, IR::Nature::temporary);
    }
  } else if (valTy->isFloat() || (valTy->isCType() && valTy->asCType()->getSubType()->isFloat())) {
    typeCheck(valTy);
    if (irVal->isPrerunValue()) {
      return new IR::PrerunValue(llvm::cast<llvm::Constant>(ctx->builder.CreateFNeg(irVal->getLLVMConstant())), valTy);
    } else {
      irVal->loadImplicitPointer(ctx->builder);
      if (irVal->getType()->isReference()) {
        irVal = new IR::Value(ctx->builder.CreateLoad(valTy->getLLVMType(), irVal->getLLVM()), valTy, false,
                              IR::Nature::temporary);
      }
      return new IR::Value(ctx->builder.CreateFSub(llvm::ConstantFP::getZero(valTy->getLLVMType()), irVal->getLLVM()),
                           valTy, false, IR::Nature::temporary);
    }
  } else if (valTy->isUnsignedInteger() || (valTy->isCType() && valTy->asCType()->getSubType()->isUnsignedInteger())) {
    ctx->Error("The value of this expression is of the unsigned integer type " +
                   ctx->highlightError(valTy->toString()) + " and cannot use the " + ctx->highlightError("unary -") +
                   " operator",
               fileRange);
  } else if (valTy->isExpanded()) {
    // FIXME - Prerun expanded type values
    if (valTy->asExpanded()->hasUnaryOperator("-")) {
      auto localID = irVal->getLocalID();
      if (irVal->getType()->isReference() || irVal->isImplicitPointer()) {
        if (irVal->getType()->isReference()) {
          irVal->loadImplicitPointer(ctx->builder);
        }
      } else {
        auto* loc = ctx->getActiveFunction()->getBlock()->newValue(utils::unique_id(), valTy, true, fileRange);
        ctx->builder.CreateStore(irVal->getLLVM(), loc->getLLVM());
        irVal = loc;
      }
      auto opFn   = valTy->asExpanded()->getUnaryOperator("-");
      auto result = opFn->call(ctx, {irVal->getLLVM()}, localID, ctx->getMod());
      typeCheck(result->getType());
      return result;
    } else {
      ctx->Error("Type " + ctx->highlightError(valTy->toString()) + " does not have the " +
                     ctx->highlightError("unary -") + " operator",
                 fileRange);
    }
  } else {
    // FIXME - Add default skill support
    ctx->Error("Type " + ctx->highlightError(valTy->toString()) + " does not support the " +
                   ctx->highlightError("unary -") + " operator",
               fileRange);
  }
}

Json Negative::toJson() const {
  return Json()._("nodeType", "negative")._("subExpression", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast