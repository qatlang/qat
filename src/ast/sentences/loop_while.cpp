#include "./loop_while.hpp"

namespace qat::ast {

LoopWhile::LoopWhile(Expression *_condition, Block *_block, Block *_after,
                     utils::FileRange _fileRange)
    : Sentence(_fileRange), block(_block), after(_after),
      condition(_condition) {}

IR::Value *LoopWhile::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json LoopWhile::toJson() const {
  return nuo::Json()
      ._("nodeType", "loopWhile")
      ._("condition", condition->toJson())
      ._("block", block->toJson())
      ._("after", after->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast