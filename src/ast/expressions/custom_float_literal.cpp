#include "./custom_float_literal.hpp"

namespace qat::ast {

CustomFloatLiteral::CustomFloatLiteral(String _value, String _kind,
                                       utils::FileRange _fileRange)
    : value(std::move(_value)), kind(std::move(_kind)),
      Expression(std::move(_fileRange)) {}

IR::Value *CustomFloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Float literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json CustomFloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "customFloatLiteral")
      ._("nature", kind)
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast