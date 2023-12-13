#include "./radix_literal.hpp"

namespace qat::ast {

RadixLiteral::RadixLiteral(String _value, u64 _radix, FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)), radix(_radix) {}

IR::Value* RadixLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("This expression is not assignable", fileRange);
  }
  u64 bitWidth = 0;
  if (radix == 2) {
    bitWidth = value.length() - 2;
  } else if (radix == 8) {
    bitWidth = (value.length() - 2) * 3;
  } else if (radix == 16) {
    bitWidth = (value.length() - 2) * 4;
  } else {
    ctx->Error("Unsupported radix", fileRange);
  }
  if (bitWidth == 0) {
    ctx->Error("No numbers provided for radix string", fileRange);
  }
  // TODO - Implement this
}

Json RadixLiteral::toJson() const {
  return Json()._("nodeType", "radixLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast