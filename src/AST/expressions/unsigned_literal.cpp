#include "./unsigned_literal.hpp"

namespace qat {
namespace AST {

UnsignedLiteral::UnsignedLiteral(std::string _value,
                                 utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *UnsignedLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Unsigned literals are not assignable", file_placement);
  }
  // TODO - Implement this
}

void UnsignedLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += value + "u";
  }
}

backend::JSON UnsignedLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "unsignedLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat