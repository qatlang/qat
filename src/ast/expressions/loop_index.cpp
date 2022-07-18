#include "./loop_index.hpp"

namespace qat::ast {

LoopIndex::LoopIndex(utils::FileRange _fileRange) : Expression(_fileRange) {}

IR::Value *LoopIndex::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void LoopIndex::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (file.getLoopIndex() + " ");
  }
}

nuo::Json LoopIndex::toJson() const {
  return nuo::Json()._("nodeType", "loopIndex");
}

} // namespace qat::ast