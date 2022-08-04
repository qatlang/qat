#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, utils::FileRange _fileRange)
    : value(std::move(_value)), Expression(std::move(_fileRange)) {}

IR::Value *IntegerLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json IntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "integerLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast