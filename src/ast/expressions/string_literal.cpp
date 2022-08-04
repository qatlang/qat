#include "./string_literal.hpp"

namespace qat::ast {

StringLiteral::StringLiteral(String _value, utils::FileRange _fileRange)
    : value(std::move(_value)), Expression(std::move(_fileRange)) {}

String StringLiteral::get_value() const { return value; }

IR::Value *StringLiteral::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json StringLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "stringLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast