#include "./block.hpp"
#include "./function.hpp"
#include "llvm_helper.hpp"

namespace qat::IR {

Block::Block(Function *_fn, Block *_parent) : fn(_fn), parent(_parent) {}

bool Block::hasParent() const { return (parent != nullptr); }

Block *Block::getParent() { return parent; }

Block *Block::getEntryBlock() { return fn->getEntryBlock(); }

void Block::setParent(Block *value) { parent = value; }

Function *Block::getFunction() { return fn; }

void Block::setActive() { fn->setCurrentBlock(this); }

bool Block::hasLocalValue(String name) const {
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

bool Block::isSame(Block *other) const { return (this->isID(other->getID())); }

LocalValue *Block::getLocalValue(String name) {
  for (usize i = 0; i < locals.size(); i++) {
    if (locals.at(i)->getName() == name) {
      return locals.at(i);
    }
  }
  if (hasParent()) {
    return parent->getLocalValue(name);
  }
  return nullptr;
}

void Block::defineLLVM(llvmHelper &help) const {}

void Block::defineCPP(cpp::File &file) const {}

llvm::Value *Block::emitLLVM(llvmHelper &help) const {}

void Block::emitCPP(cpp::File &file) const {
  if (file.isSourceFile()) {
    if (!file.getOpenBlock()) {
      file << "{\n";
    }
    // FIXME
    if (!file.getOpenBlock()) {
      file << "\n}\n";
      file.setOpenBlock(false);
    }
  }
}

nuo::Json Block::toJson() const {}

} // namespace qat::IR