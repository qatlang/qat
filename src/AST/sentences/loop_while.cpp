#include "./loop_while.hpp"

namespace qat::AST {

LoopWhile::LoopWhile(Expression *_condition, Block *_block, Block *_after,
                     utils::FilePlacement _filePlacement)
    : Sentence(_filePlacement), block(_block), after(_after),
      condition(_condition) {}

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

nuo::Json LoopWhile::toJson() const {
  return nuo::Json()
      ._("nodeType", "loopWhile")
      ._("condition", condition->toJson())
      ._("block", block->toJson())
      ._("after", after->toJson())
      ._("filePlacement", file_placement);
}

} // namespace qat::AST