#include "./radix_literal.hpp"

namespace qat::ast {

RadixLiteral::RadixLiteral(std::string _value, unsigned _radix,
                           utils::FileRange _fileRange)
    : value(_value), radix(_radix), Expression(_fileRange) {}

IR::Value *RadixLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("This expression is not assignable", fileRange);
  }
  unsigned bitWidth = 0;
  if (radix == 2) {
    bitWidth = value.length() - 2;
  } else if (radix == 8) {
    bitWidth = (value.length() - 2) * 3;
  } else if (radix == 16) {
    bitWidth = (value.length() - 2) * 4;
  } else {
    ctx->throw_error("Unsupported radix", fileRange);
  }
  if (bitWidth == 0) {
    ctx->throw_error("No numbers provided for radix string", fileRange);
  }
  // TODO - Implement this
}

void RadixLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    std::string val;
    if (radix == 2) {
      val = "0b" + value;
    } else if (radix == 8) {
      val = "0" + value;
    } else if (radix == 16) {
      val = "0x" + value;
    } else {
      val = value;
    }
    file += (val + " ");
  }
}

nuo::Json RadixLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "radixLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast