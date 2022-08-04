#include "./float_literal.hpp"

namespace qat::ast {

FloatLiteral::FloatLiteral(String _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)) {}

IR::Value *FloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Float literals are not assignable", fileRange);
  }
  // FIXME - Change
  return nullptr;
}

nuo::Json FloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "floatLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast