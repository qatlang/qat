#include "./loop_index.hpp"

namespace qat {
namespace AST {

LoopIndex::LoopIndex(utils::FilePlacement _filePlacement)
    : Expression(_filePlacement) {}

IR::Value *LoopIndex::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void LoopIndex::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (file.getLoopIndex() + " ");
  }
}

} // namespace AST
} // namespace qat