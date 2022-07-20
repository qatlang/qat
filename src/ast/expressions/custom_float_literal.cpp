#include "./custom_float_literal.hpp"

namespace qat::ast {

CustomFloatLiteral::CustomFloatLiteral(std::string _value, std::string _kind,
                                       utils::FileRange _fileRange)
    : value(_value), kind(_kind), Expression(_fileRange) {}

IR::Value *CustomFloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Float literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json CustomFloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "customFloatLiteral")
      ._("kind", kind)
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast