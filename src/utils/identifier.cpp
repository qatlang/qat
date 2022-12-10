#include "./identifier.hpp"

namespace qat {

Identifier::Identifier(String _value, FileRange _fileRange) : value(std::move(_value)), range(std::move(_fileRange)) {}

Identifier::operator JsonValue() const { return Json()._("value", value)._("fileRange", range); }

Identifier Identifier::fullName(Vec<Identifier> ids) {
  auto name  = ids.front().value;
  auto range = ids.front().range;
  for (usize i = 1; i < ids.size(); i++) {
    name.append(":").append(ids.at(i).value);
    range = {range, ids.at(i).range};
  }
  return {name, range};
}

} // namespace qat