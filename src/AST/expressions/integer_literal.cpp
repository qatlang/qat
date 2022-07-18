#include "./integer_literal.hpp"

namespace qat {
namespace AST {

IntegerLiteral::IntegerLiteral(std::string _value,
                               utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *IntegerLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Integer literals are not assignable", file_placement);
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
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat