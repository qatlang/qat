#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bits(_bits) {}

IR::PrerunValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred() && (!inferredType->isInteger() && !inferredType->isUnsignedInteger() &&
                           (inferredType->isCType() && !(inferredType->asCType()->getSubType()->isUnsignedInteger() ||
                                                         inferredType->asCType()->getSubType()->isInteger())))) {
    ctx->Error("The inferred type of this expression is " + inferredType->toString() +
                   ". The only supported types in type inference for integer literal are signed & unsigned integers",
               fileRange);
  }
  if (bits.has_value() && !ctx->getMod()->hasIntegerBitwidth(bits.value().first)) {
    ctx->Error("The custom integer bitwidth " + ctx->highlightError(std::to_string(bits.value().first)) +
                   " is not brought into the current module",
               bits.value().second);
  }
  String intValue = value;
  if (value.find('_') != String::npos) {
    intValue = "";
    for (auto intCh : value) {
      if (intCh != '_') {
        intValue += intCh;
      }
    }
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::PrerunValue(
      llvm::ConstantInt::get(isTypeInferred()
                                 ? llvm::cast<llvm::IntegerType>(inferredType->getLLVMType())
                                 : llvm::Type::getIntNTy(ctx->llctx, bits.has_value() ? bits.value().first : 32u),
                             intValue, 10u),
      isTypeInferred() ? inferredType : IR::IntegerType::get(bits.has_value() ? bits.value().first : 32u, ctx));
  // NOLINTEND(readability-magic-numbers)
}

String IntegerLiteral::toString() const { return value; }

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast