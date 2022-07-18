#include "./unsigned_literal.hpp"

namespace qat::AST {

UnsignedLiteral::UnsignedLiteral(std::string _value,
                                 utils::FileRange _filePlacement)
    : value(_value), Expression(_filePlacement) {}

IR::Value *UnsignedLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Unsigned literals are not assignable", fileRange);
  }
  // TODO - Implement this
}

void UnsignedLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += value + "u";
  }
}

nuo::Json UnsignedLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "unsignedLiteral")
      ._("value", value)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST