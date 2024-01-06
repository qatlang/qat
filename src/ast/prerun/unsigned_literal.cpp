#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bits(_bits) {}

IR::PrerunValue* UnsignedLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred() && !inferredType->isUnsignedInteger() &&
      (inferredType->isCType() && !inferredType->asCType()->getSubType()->isUnsignedInteger())) {
    ctx->Error("The inferred type of this expression is " + inferredType->toString() +
                   " which is not an unsigned integer type",
               fileRange);
  }
  if (bits.has_value() && !ctx->getMod()->hasUnsignedBitwidth(bits.value().first)) {
    ctx->Error("The custom unsigned integer bitwidth " + ctx->highlightError(std::to_string(bits.value().first)) +
                   " is not brought into the module",
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
                                 ? llvm::dyn_cast<llvm::IntegerType>(inferredType->getLLVMType())
                                 : llvm::Type::getIntNTy(ctx->llctx, bits.has_value() ? bits.value().first : 32u),
                             intValue, 10u),
      isTypeInferred() ? inferredType : IR::UnsignedType::get(bits.has_value() ? bits.value().first : 32u, ctx));
  // NOLINTEND(readability-magic-numbers)
}

Json UnsignedLiteral::toJson() const {
  return Json()._("nodeType", "unsignedLiteral")._("value", value)._("fileRange", fileRange);
}

String UnsignedLiteral::toString() const { return value; }

} // namespace qat::ast