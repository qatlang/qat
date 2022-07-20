#include "./if_else.hpp"

namespace qat::ast {

IfElse::IfElse(Expression *_condition, Block *_if_block,
               std::optional<Block *> _else_block,
               std::optional<Block *> _merge_block, utils::FileRange _fileRange)
    : Sentence(_fileRange), condition(_condition), if_block(_if_block),
      else_block(_else_block),
      merge_block(_merge_block.value_or(new Block(
          std::vector<Sentence *>(),
          utils::FileRange(_else_block.value_or(_if_block)->fileRange.file,
                           _else_block.value_or(_if_block)->fileRange.end,
                           _else_block.value_or(_if_block)->fileRange.end)))) {}

IR::Value *IfElse::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json IfElse::toJson() const {
  return nuo::Json()
      ._("nodeType", "ifElse")
      ._("condition", condition->toJson())
      ._("ifBlock", if_block->toJson())
      ._("hasElse", else_block.has_value())
      ._("elseBlock",
         else_block.has_value() ? else_block.value()->toJson() : nuo::Json())
      ._("mergeBlock", merge_block->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast