#include "./custom_integer_literal.hpp"
#include <nuo/json.hpp>

namespace qat::ast {

CustomIntegerLiteral::CustomIntegerLiteral(String _value, bool _isUnsigned,
                                           u32              _bitWidth,
                                           utils::FileRange _fileRange)
    : value(_value), isUnsigned(_isUnsigned), bitWidth(_bitWidth),
      Expression(_fileRange) {}

IR::Value *CustomIntegerLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json CustomIntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "customIntegerLiteral")
      ._("isUnsigned", isUnsigned)
      ._("bitWidth", (u64)bitWidth)
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast