#include "./loop_while.hpp"

namespace qat {
namespace AST {

LoopWhile::LoopWhile(Expression *_condition, Block *_block, Block *_after,
                     utils::FilePlacement _filePlacement)
    : condition(_condition), block(_block), after(_after),
      Sentence(_filePlacement) {}

IR::Value *LoopWhile::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void LoopWhile::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += "while (";
  condition->emitCPP(file, isHeader);
  file += ")";
  block->emitCPP(file, isHeader);
  file.setOpenBlock(true);
  after->emitCPP(file, isHeader);
}

} // namespace AST
} // namespace qat