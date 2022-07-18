#include "./string_literal.hpp"

namespace qat::AST {

StringLiteral::StringLiteral(std::string _value,
                             utils::FileRange _filePlacement)
    : value(_value), Expression(_filePlacement) {}

std::string StringLiteral::get_value() const { return value; }

IR::Value *StringLiteral::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void StringLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "\"" + value + "\" ";
  }
}

nuo::Json StringLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "stringLiteral")
      ._("value", value)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST