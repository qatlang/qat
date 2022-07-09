#include "./string_literal.hpp"

namespace qat {
namespace AST {

StringLiteral::StringLiteral(std::string _value,
                             utils::FilePlacement _filePlacement)
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

backend::JSON StringLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "stringLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat