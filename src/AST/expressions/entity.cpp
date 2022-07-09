#include "./entity.hpp"

namespace qat {
namespace AST {

IR::Value *Entity::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void Entity::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    auto value = name;
    if (value.find("..:") == 0) {
      while (value.find("..:") == 0) {
        value = name.substr(3);
      }
    }
    file += (value + " ");
  }
}

backend::JSON Entity::toJSON() const {
  return backend::JSON()
      ._("nodeType", "entity")
      ._("name", name)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat