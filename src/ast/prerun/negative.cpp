#include "./negative.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

PrerunNegative::PrerunNegative(PrerunExpression* _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(_value) {}

IR::PrerunValue* PrerunNegative::emit(IR::Context* ctx) {
  if (isTypeInferred()) {
    if (value->hasTypeInferrance()) {
      value->asTypeInferrable()->setInferenceType(inferredType);
    }
  }
  auto* irVal = value->emit(ctx);
  if (irVal->getType()->isInteger() ||
      (irVal->getType()->isCType() && irVal->getType()->asCType()->getSubType()->isInteger())) {
    return new IR::PrerunValue(
        llvm::ConstantFoldConstant(llvm::ConstantExpr::getNeg(irVal->getLLVMConstant()), ctx->dataLayout.value()),
        irVal->getType());
  } else if (irVal->getType()->isFloat() ||
             (irVal->getType()->isCType() && irVal->getType()->asCType()->getSubType()->isFloat())) {
    return new IR::PrerunValue(llvm::cast<llvm::Constant>(ctx->builder.CreateFNeg(irVal->getLLVMConstant())),
                               irVal->getType());
  } else if (irVal->getType()->isUnsignedInteger() ||
             (irVal->getType()->isCType() && irVal->getType()->asCType()->getSubType()->isInteger())) {
    ctx->Error("Expression of unsigned integer type " + ctx->highlightError(irVal->getType()->toString()) +
                   " cannot use the " + ctx->highlightError("unary -") + " operator",
               fileRange);
  } else {
    // FIXME - Support prerun operator functions
    ctx->Error("Type " + ctx->highlightError(irVal->getType()->toString()) + " does not support the " +
                   ctx->highlightError("unary -") + " operator",
               fileRange);
  }
  return nullptr;
}

Json PrerunNegative::toJson() const {
  return Json()._("nodeType", "prerunNegative")._("subExpression", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast