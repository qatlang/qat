#include "./identifier.hpp"

namespace qat {

Identifier::Identifier(String _value, utils::FileRange _fileRange)
    : value(std::move(_value)), range(std::move(_fileRange)) {}

Identifier::operator JsonValue() const { return Json()._("value", value)._("fileRange", range); }

} // namespace qat