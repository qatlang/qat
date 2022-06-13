#include "./loop_n_times.hpp"

namespace qat {
namespace AST {

LoopNTimes::LoopNTimes(Expression _count, Block _block, Block _after,
                       utils::FilePlacement _filePlacement)
    : count(_count), block(_block), after(_after), Sentence(_filePlacement) {}

unsigned LoopNTimes::new_loop_index_id(llvm::BasicBlock *bb) {
  unsigned result = 0;
  bb = &bb->getParent()->getEntryBlock();
  for (auto inst = bb->begin(); inst != bb->end(); inst++) {
    if (llvm::isa<llvm::AllocaInst>(*inst)) {
      auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&*inst);
      if (alloc->getName().str().find("loop:times:index:") !=
          std::string::npos) {
        result++;
      }
    } else {
      break;
    }
  }
  return result;
}

llvm::Value *LoopNTimes::generate(IR::Generator *generator) {
  auto count_val = count.generate(generator);
  if (!count_val->getType()->isIntegerTy()) {
    generator->throw_error("Expected an expression of integer type for the "
                           "number of times to loop",
                           count.file_placement);
  }
  auto prev_bb = generator->builder.GetInsertBlock();
  auto inst_iter =
      &generator->builder.GetInsertBlock()->getParent()->getEntryBlock();
  llvm::Instruction *last_inst;
  for (auto inst = inst_iter->begin(); inst != inst_iter->end(); inst++) {
    if (!llvm::isa<llvm::AllocaInst>(*inst)) {
      last_inst = &*inst;
    }
  }
  generator->builder.SetInsertPoint(last_inst);
  auto loop_bb = block.create_bb(generator);
  auto id =
      std::to_string(new_loop_index_id(generator->builder.GetInsertBlock()));
  auto index_alloca = generator->builder.CreateAlloca(
      count_val->getType(), 0, nullptr, "loop:times:index:" + id);
  index_alloca->setMetadata(
      "origin_block",
      llvm::MDNode::get(
          generator->llvmContext,
          llvm::MDString::get(generator->llvmContext, loop_bb->getName())));
  utils::Variability::set(generator->llvmContext, index_alloca, true);
  generator->builder.SetInsertPoint(prev_bb);
  /**
   * @brief Storing the initial value of the index
   *
   */
  generator->builder.CreateStore(
      llvm::ConstantInt::get(
          llvm::dyn_cast<llvm::IntegerType>(count_val->getType()), 0U, false),
      index_alloca, false);
  /**
   * @brief Generate IR for all sentences present within the loop
   *
   */
  block.generate(generator);
  /**
   * @brief The ending BasicBlock generated might be different
   *
   */
  auto loop_end_bb = generator->builder.GetInsertBlock();
  index_alloca->setMetadata(
      "end_block",
      llvm::MDNode::get(
          generator->llvmContext,
          llvm::MDString::get(generator->llvmContext, loop_end_bb->getName())));
  generator->builder.CreateStore(
      generator->builder.CreateAdd(
          generator->builder.CreateLoad(index_alloca),
          llvm::ConstantInt::get(
              llvm::dyn_cast<llvm::IntegerType>(count_val->getType()), 1U,
              false) //
          ),
      index_alloca, false //
  );
  auto after_bb = after.create_bb(generator);
  generator->builder.CreateCondBr(
      generator->builder.CreateICmpULT(
          generator->builder.CreateLoad(index_alloca), count_val,
          "loop:times:check:" + id),
      loop_bb, after_bb);
  auto after_end = after.generate(generator);
  auto after_end_bb = generator->builder.GetInsertBlock();
  generator->builder.SetInsertPoint(prev_bb);
  // FIXME - The count could be signed or unsigned integer, change logic to
  // adapt to that
  generator->builder.CreateCondBr(
      generator->builder.CreateICmpSGE(
          count_val, llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(
                                                count_val->getType()),
                                            1U, true)),
      loop_bb, after_bb);
  generator->builder.SetInsertPoint(after_end_bb);
  return after_end;
}

} // namespace AST
} // namespace qat