#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)) {}

IR::Value *IntegerLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::Value(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), value, 10u),
      IR::IntegerType::get(32, ctx->llctx), false, IR::Nature::pure);
  // NOLINTEND(readability-magic-numbers)
}

nuo::Json IntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "integerLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast