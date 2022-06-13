#include "./loop_while.hpp"

namespace qat {
namespace AST {

LoopWhile::LoopWhile(Expression _condition, Block _block, Block _after,
                     utils::FilePlacement _filePlacement)
    : condition(_condition), block(_block), after(_after),
      Sentence(_filePlacement) {}

llvm::Value *LoopWhile::generate(IR::Generator *generator) {
  auto cond_val = condition.generate(generator);
  if (cond_val->getType()->isIntegerTy()) {
    auto bitWidth =
        llvm::dyn_cast<llvm::IntegerType>(cond_val->getType())->getBitWidth();
    if (bitWidth != 1) {
      generator->throw_error("The integer type is not i1 or u1",
                             condition.file_placement);
    }
  } else if (!llvm::isa<llvm::BranchInst>(cond_val)) {
    generator->throw_error("Expected an expression of i1, bool or u1 type",
                           condition.file_placement);
  }
  auto prev_bb = generator->builder.GetInsertBlock();
  auto loop_bb = block.create_bb(generator);
  /**
   * @brief Generate IR for all sentences present within the loop
   *
   */
  block.generate(generator);
  auto loop_end_bb = generator->builder.GetInsertBlock();
  auto after_bb = after.create_bb(generator);
  generator->builder.SetInsertPoint(loop_end_bb);
  generator->builder.CreateCondBr(condition.generate(generator), loop_bb,
                                  after_bb);
  generator->builder.SetInsertPoint(prev_bb);
  /**
   * @brief Since this is looping as long as the condition is true, there should
   * be a check in the beginning of the first loop execution
   *
   */
  generator->builder.CreateCondBr(cond_val, loop_bb, after_bb);
  return after.generate(generator);
}

} // namespace AST
} // namespace qat