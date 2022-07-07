#include "./loop_while.hpp"

namespace qat {
namespace AST {

LoopWhile::LoopWhile(Expression *_condition, Block *_block, Block *_after,
                     utils::FilePlacement _filePlacement)
    : condition(_condition), block(_block), after(_after),
      Sentence(_filePlacement) {}

llvm::Value *LoopWhile::emit(IR::Context *ctx) {
  auto cond_val = condition->emit(ctx);
  if (cond_val->getType()->isIntegerTy()) {
    auto bitWidth =
        llvm::dyn_cast<llvm::IntegerType>(cond_val->getType())->getBitWidth();
    if (bitWidth != 1) {
      ctx->throw_error("The integer type is not i1 or u1",
                       condition->file_placement);
    }
  } else if (!llvm::isa<llvm::BranchInst>(cond_val)) {
    ctx->throw_error("Expected an expression of i1, bool or u1 type",
                     condition->file_placement);
  }
  auto prev_bb = ctx->builder.GetInsertBlock();
  auto loop_bb = block->create_bb(ctx);
  /**
   *  Generate IR for all sentences present within the loop
   *
   */
  block->emit(ctx);
  auto loop_end_bb = ctx->builder.GetInsertBlock();
  auto after_bb = after->create_bb(ctx);
  ctx->builder.SetInsertPoint(loop_end_bb);
  ctx->builder.CreateCondBr(condition->emit(ctx), loop_bb, after_bb);
  ctx->builder.SetInsertPoint(prev_bb);
  /**
   *  Since this is looping as long as the condition is true, there should
   * be a check in the beginning of the first loop execution
   *
   */
  ctx->builder.CreateCondBr(cond_val, loop_bb, after_bb);
  return after->emit(ctx);
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