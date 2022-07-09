#include "./block.hpp"
#include "./function.hpp"

namespace qat {
namespace IR {

Block::Block(Function *_fn, Block *_parent)
    : id(utils::unique_id()), fn(_fn), parent(_parent) {}

bool Block::hasParent() const { return (parent != nullptr); }

Block *Block::getParent() { return parent; }

Block *Block::getEntryBlock() { fn->getEntryBlock(); }

void Block::setParent(Block *value) { parent = value; }

Function *Block::getFunction() { return fn; }

void Block::setActive() { fn->setCurrentBlock(this); }

bool Block::hasLocalValue(std::string name) const {
  for (auto val : locals) {
    if (val->getName() == name) {
      return true;
    }
  }
  if (hasParent()) {
    return parent->hasLocalValue(name);
  } else {
    return false;
  }
}

bool Block::isSame(Block *other) const { return (this->id == other->id); }

LocalValue *Block::getLocalValue(std::string name) {
  for (std::size_t i = 0; i < locals.size(); i++) {
    if (locals.at(i)->getName() == name) {
      return locals.at(i);
    }
  }
  if (hasParent()) {
    return parent->getLocalValue(name);
  }
}

} // namespace IR
} // namespace qat