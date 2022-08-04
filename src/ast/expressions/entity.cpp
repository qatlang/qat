#include <utility>

#include "./entity.hpp"

namespace qat::ast {

Entity::Entity(String _name, utils::FileRange _fileRange)
    : name(std::move(_name)), Expression(std::move(_fileRange)) {}

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