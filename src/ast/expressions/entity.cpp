#include "./entity.hpp"

namespace qat::ast {

IR::Value *Entity::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json Entity::toJson() const {
  return nuo::Json()
      ._("nodeType", "entity")
      ._("name", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast