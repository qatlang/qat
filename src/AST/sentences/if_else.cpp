#include "./if_else.hpp"

namespace qat::AST {

IfElse::IfElse(Expression *_condition, Block *_if_block,
               std::optional<Block *> _else_block,
               std::optional<Block *> _merge_block,
               utils::FilePlacement _filePlacement)
    : Sentence(_filePlacement), condition(_condition), if_block(_if_block),
      else_block(_else_block),
      merge_block(_merge_block.value_or(new Block(
          std::vector<Sentence *>(),
          utils::FilePlacement(
              _else_block.value_or(_if_block)->file_placement.file,
              _else_block.value_or(_if_block)->file_placement.end,
              _else_block.value_or(_if_block)->file_placement.end)))) {}

IR::Value *IfElse::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void IfElse::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "if (";
    condition->emitCPP(file, isHeader);
    file += ") ";
    if_block->emitCPP(file, isHeader);
    if (else_block.has_value()) {
      file += " else ";
      else_block.value()->emitCPP(file, isHeader);
    }
    file.setOpenBlock(true);
    merge_block->emitCPP(file, isHeader);
  }
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
      ._("filePlacement", file_placement);
}

} // namespace qat::AST