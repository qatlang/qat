#include "./float_literal.hpp"

namespace qat::ast {

FloatLiteral::FloatLiteral(std::string _value, utils::FileRange _fileRange)
    : value(_value), Expression(_fileRange) {}

IR::Value *FloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Float literals are not assignable", fileRange);
  }
  //
}

nuo::Json FloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "floatLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast