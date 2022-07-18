#include "./entity.hpp"

namespace qat::ast {

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

nuo::Json Entity::toJson() const {
  return nuo::Json()
      ._("nodeType", "entity")
      ._("name", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast