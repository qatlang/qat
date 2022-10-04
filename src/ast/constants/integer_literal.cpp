#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, utils::FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::ConstantValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::ConstantValue(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), value, 10u),
                               IR::UnsignedType::get(32, ctx->llctx));
  // NOLINTEND(readability-magic-numbers)
}

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast