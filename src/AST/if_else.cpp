#include "./if_else.hpp"

namespace qat {
namespace AST {

IfElse::IfElse(Expression *_condition, Block *_if_block,
               std::optional<Block *> _else_block,
               std::optional<Block *> _merge_block,
               utils::FilePlacement _filePlacement)
    : condition(_condition), if_block(_if_block), else_block(_else_block),
      merge_block(_merge_block.value_or(
          new Block(std::vector<Sentence *>(),
                    utils::FilePlacement(
                        _else_block.value_or(_if_block)->file_placement.file,
                        _else_block.value_or(_if_block)->file_placement.end,
                        _else_block.value_or(_if_block)->file_placement.end)))),
      Sentence(_filePlacement) {}

llvm::Value *IfElse::emit(IR::Context *ctx) {
  auto gen_cond = condition->emit(ctx);
  if (gen_cond) {
    if (gen_cond->getType()->isIntegerTy(1)) {
      auto if_bb = if_block->create_bb(ctx);
      auto cond_value = ctx->builder.CreateICmpEQ(
          gen_cond,
          llvm::ConstantInt::get(ctx->llvmContext, llvm::APInt(1u, 1u, false)),
          if_bb->getName() + "'condition");
      auto else_bb =
          else_block.has_value() ? else_block.value()->create_bb(ctx) : nullptr;
      auto merge_bb = merge_block->create_bb(ctx);
      ctx->builder.CreateCondBr(cond_value, if_bb, else_bb);

      /* If */
      if_block->emit(ctx);
      ctx->builder.CreateBr(merge_bb);

      /* Else */
      if (else_block.has_value()) {
        else_block.value()->emit(ctx);
        ctx->builder.CreateBr(merge_bb);
      }

      /* Merge - Remaining sentences not part of the conditional branching */
      return merge_block->emit(ctx);
    } else {
      ctx->throw_error(
          "Condition expression is of the type `" +
              qat::utils::llvmTypeToName(gen_cond->getType()) +
              "`, but `if` sentence expects an expression of `bool`, "
              "`i1` or `u1` type",
          file_placement);
    }
  } else {
    ctx->throw_error("Condition expression is null, but `if` sentence "
                     "expects an expression of `bool`, `i1` or `u1` type",
                     file_placement);
  }
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

backend::JSON IfElse::toJSON() const {
  return backend::JSON()
      ._("nodeType", "ifElse")
      ._("condition", condition->toJSON())
      ._("ifBlock", if_block->toJSON())
      ._("hasElse", else_block.has_value())
      ._("elseBlock", else_block.has_value() ? else_block.value()->toJSON()
                                             : backend::JSON())
      ._("mergeBlock", merge_block->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat