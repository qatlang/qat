#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::PrerunValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  if (inferredType && (!inferredType.value()->isInteger() && !inferredType.value()->isUnsignedInteger())) {
    ctx->Error("The inferred type of this expression is " + inferredType.value()->toString() +
                   ". The only supported types in type inference for integer literal are signed & unsigned integers",
               fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::PrerunValue(llvm::ConstantInt::get(inferredType
                                                        ? (llvm::IntegerType*)(inferredType.value()->getLLVMType())
                                                        : llvm::Type::getInt32Ty(ctx->llctx),
                                                    value, 10u),
                             inferredType ? inferredType.value() : IR::IntegerType::get(32, ctx->llctx));
  // NOLINTEND(readability-magic-numbers)
}

String IntegerLiteral::toString() const { return value; }

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast