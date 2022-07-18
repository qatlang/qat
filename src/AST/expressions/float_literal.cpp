#include "./float_literal.hpp"

namespace qat::AST {

FloatLiteral::FloatLiteral(std::string _value, utils::FileRange _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *FloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Float literals are not assignable", fileRange);
  }
  //
}

void FloatLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (value + " ");
  }
}

nuo::Json FloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "floatLiteral")
      ._("value", value)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST