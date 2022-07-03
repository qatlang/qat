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

llvm::Value *IfElse::generate(IR::Generator *generator) {
  auto gen_cond = condition->generate(generator);
  if (gen_cond) {
    if (gen_cond->getType()->isIntegerTy(1)) {
      auto if_bb = if_block->create_bb(generator);
      auto cond_value = generator->builder.CreateICmpEQ(
          gen_cond,
          llvm::ConstantInt::get(generator->llvmContext,
                                 llvm::APInt(1u, 1u, false)),
          if_bb->getName() + "'condition");
      auto else_bb = else_block.has_value()
                         ? else_block.value()->create_bb(generator)
                         : nullptr;
      auto merge_bb = merge_block->create_bb(generator);
      generator->builder.CreateCondBr(cond_value, if_bb, else_bb);

      /* If */
      if_block->generate(generator);
      generator->builder.CreateBr(merge_bb);

      /* Else */
      if (else_block.has_value()) {
        else_block.value()->generate(generator);
        generator->builder.CreateBr(merge_bb);
      }

      /* Merge - Remaining sentences not part of the conditional branching */
      return merge_block->generate(generator);
    } else {
      generator->throw_error(
          "Condition expression is of the type `" +
              qat::utils::llvmTypeToName(gen_cond->getType()) +
              "`, but `if` sentence expects an expression of `bool`, "
              "`i1` or `u1` type",
          file_placement);
    }
  } else {
    generator->throw_error("Condition expression is null, but `if` sentence "
                           "expects an expression of `bool`, `i1` or `u1` type",
                           file_placement);
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