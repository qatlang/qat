#include "./integer_literal.hpp"

namespace qat::ast {

IR::PrerunValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred() &&
      (!inferredType->isInteger() && !inferredType->isUnsignedInteger() &&
       (!bits.has_value() && !inferredType->isFloat()) &&
       (!bits.has_value() && inferredType->isCType() && !inferredType->asCType()->getSubType()->isFloat()) &&
       (inferredType->isCType() && !(inferredType->asCType()->getSubType()->isUnsignedInteger() ||
                                     inferredType->asCType()->getSubType()->isInteger())))) {
    if (bits.has_value()) {
      ctx->Error("The inferred type is " + ctx->highlightError(inferredType->toString()) +
                     ". The only supported types for this literal are signed & unsigned integers",
                 fileRange);
    } else {
      ctx->Error(
          "This inferred type is " + ctx->highlightError(inferredType->toString()) +
              ". The only supported types for this literal are signed integers, unsigned integers and floating point types",
          fileRange);
    }
  }
  IR::QatType* resTy = isTypeInferred()
                           ? (inferredType->isCType() ? inferredType->asCType()->getSubType() : inferredType)
                           : IR::IntegerType::get(32, ctx);
  if (bits.has_value() && !ctx->getMod()->hasIntegerBitwidth(bits.value().first)) {
    ctx->Error("The custom integer bitwidth " + ctx->highlightError(std::to_string(bits.value().first)) +
                   " is not brought into the current module",
               bits.value().second);
  }
  String numValue = value;
  if (value.find('_') != String::npos) {
    numValue = "";
    for (auto intCh : value) {
      if (intCh != '_') {
        numValue += intCh;
      }
    }
  }
  // NOLINTBEGIN(readability-magic-numbers)
  if (resTy->isFloat()) {
    return new IR::PrerunValue(llvm::ConstantFP::get(inferredType->getLLVMType(), numValue), inferredType);
  } else {
    return new IR::PrerunValue(
        llvm::ConstantInt::get(isTypeInferred()
                                   ? llvm::cast<llvm::IntegerType>(inferredType->getLLVMType())
                                   : llvm::Type::getIntNTy(ctx->llctx, bits.has_value() ? bits.value().first : 32u),
                               numValue, 10u),
        isTypeInferred() ? inferredType : IR::IntegerType::get(bits.has_value() ? bits.value().first : 32u, ctx));
  }
  // NOLINTEND(readability-magic-numbers)
}

String IntegerLiteral::toString() const { return value; }

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast