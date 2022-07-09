#include "./float_literal.hpp"

namespace qat {
namespace AST {

FloatLiteral::FloatLiteral(std::string _value,
                           utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *FloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->throw_error("Float literals are not assignable", file_placement);
  }
  //
}

void FloatLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (value + " ");
  }
}

backend::JSON FloatLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "floatLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat