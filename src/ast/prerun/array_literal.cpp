#include "./array_literal.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IR::PrerunValue* PrerunArrayLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred()) {
    if (!inferredType->isArray()) {
      ctx->Error("This expression expects an array type, but the type inferred from scope is " +
                     ctx->highlightError(inferredType->toString()),
                 fileRange);
    }
    if (inferredType->asArray()->getLength() != valuesExp.size()) {
      ctx->Error("The inferred type is " + ctx->highlightError(inferredType->toString()) + " expecting " +
                     ctx->highlightError(std::to_string(inferredType->asArray()->getLength())) + " elements, but " +
                     ctx->highlightError(std::to_string(valuesExp.size())) + " values were provided instead",
                 fileRange);
    }
  }
  IR::QatType*         elementType = nullptr;
  Vec<llvm::Constant*> constVals;
  for (usize i = 0; i < valuesExp.size(); i++) {
    if (isTypeInferred() && valuesExp[i]->hasTypeInferrance()) {
      valuesExp[i]->asTypeInferrable()->setInferenceType(inferredType->asArray()->getElementType());
    }
    auto itVal = valuesExp.at(i)->emit(ctx);
    if (isTypeInferred()) {
      if (!inferredType->asArray()->getElementType()->isSame(itVal->getType())) {
        ctx->Error("This expression is of type " + ctx->highlightError(itVal->getType()->toString()) +
                       " which does not match with the expected element type of the inferred type, which is " +
                       ctx->highlightError(inferredType->asArray()->getElementType()->toString()),
                   valuesExp[i]->fileRange);
      }
    } else {
      if (elementType) {
        if (!elementType->isSame(itVal->getType())) {
          ctx->Error("Type of this expression is " + ctx->highlightError(itVal->getType()->toString()) +
                         " which does not match the type of the previous elements, which is " +
                         ctx->highlightError(elementType->toString()),
                     valuesExp[i]->fileRange);
        }
      } else {
        elementType = itVal->getType();
      }
    }
    constVals.push_back(itVal->getLLVMConstant());
  }
  return new IR::PrerunValue(
      llvm::ConstantArray::get(isTypeInferred() ? llvm::cast<llvm::ArrayType>(inferredType->getLLVMType())
                                                : llvm::ArrayType::get(elementType->getLLVMType(), constVals.size()),
                               constVals),
      isTypeInferred() ? inferredType : IR::ArrayType::get(elementType, constVals.size(), ctx->llctx));
}

Json PrerunArrayLiteral::toJson() const {
  Vec<JsonValue> valuesJson;
  return Json()._("nodeType", "arrayLiteral")._("values", valuesJson)._("fileRange", fileRange);
}

} // namespace qat::ast