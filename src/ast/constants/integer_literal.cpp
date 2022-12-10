#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::ConstantValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  if (expected && (!expected->isInteger() && !expected->isUnsignedInteger())) {
    ctx->Error("The inferred type of this expression is " + expected->toString() +
                   ". The only supported types in type inference for integer literal are signed & unsigned integers",
               fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::ConstantValue(llvm::ConstantInt::get(expected ? (llvm::IntegerType*)(expected->getLLVMType())
                                                               : llvm::Type::getInt32Ty(ctx->llctx),
                                                      value, 10u),
                               expected ? expected : IR::IntegerType::get(32, ctx->llctx));
  // NOLINTEND(readability-magic-numbers)
}

void IntegerLiteral::setType(IR::QatType* typ) const { expected = typ; }

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast