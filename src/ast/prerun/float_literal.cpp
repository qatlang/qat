#include "./float_literal.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Instruction.h"

namespace qat::ast {

IR::PrerunValue* FloatLiteral::emit(IR::Context* ctx) {
  SHOW("Generating float literal for " << value)
  IR::QatType* floatResTy = nullptr;
  if (isTypeInferred()) {
    if (inferredType->isFloat() || (inferredType->isCType() && inferredType->asCType()->getSubType()->isFloat())) {
      floatResTy = inferredType;
    } else {
      ctx->Error("The type inferred from scope is " + ctx->highlightError(inferredType->toString()) +
                     " but this expression is expected to have a floating point type",
                 fileRange);
    }
  } else {
    floatResTy = IR::FloatType::get(IR::FloatTypeKind::_64, ctx->llctx);
  }
  return new IR::PrerunValue(llvm::ConstantFP::get(floatResTy->getLLVMType(), value), floatResTy);
}

String FloatLiteral::toString() const { return value; }

Json FloatLiteral::toJson() const {
  return Json()._("nodeType", "floatLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast