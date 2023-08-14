#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::ConstantValue* UnsignedLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Unsigned literals are not assignable", fileRange);
  }
  if (inferredType && !inferredType.value()->isUnsignedInteger()) {
    ctx->Error("The inferred type of this expression is " + inferredType.value()->toString() +
                   ". Unsigned integer literal expects an unsigned integer type to be provided for type inference",
               fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::ConstantValue(
      llvm::ConstantInt::get(inferredType ? llvm::dyn_cast<llvm::IntegerType>(inferredType.value()->getLLVMType())
                                          : llvm::Type::getInt32Ty(ctx->llctx),
                             value, 10u),
      inferredType ? inferredType.value() : IR::UnsignedType::get(32u, ctx->llctx));
  // NOLINTEND(readability-magic-numbers)
}

Json UnsignedLiteral::toJson() const {
  return Json()._("nodeType", "unsignedLiteral")._("value", value)._("fileRange", fileRange);
}

String UnsignedLiteral::toString() const { return value; }

} // namespace qat::ast