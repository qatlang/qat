#include "./integer_literal.hpp"

namespace qat::AST {

IntegerLiteral::IntegerLiteral(std::string _value,
                               utils::FileRange _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *IntegerLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Integer literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

void IntegerLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (value + " ");
  }
}

nuo::Json IntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "integerLiteral")
      ._("value", value)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST