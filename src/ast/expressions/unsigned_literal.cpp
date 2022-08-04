#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, utils::FileRange _fileRange)
    : value(_value), Expression(_fileRange) {}

IR::Value *UnsignedLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Unsigned literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json UnsignedLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "unsignedLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast