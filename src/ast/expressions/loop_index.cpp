#include "./loop_index.hpp"

namespace qat::ast {

LoopIndex::LoopIndex(utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)) {}

IR::Value *LoopIndex::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json LoopIndex::toJson() const {
  return nuo::Json()._("nodeType", "loopIndex");
}

} // namespace qat::ast