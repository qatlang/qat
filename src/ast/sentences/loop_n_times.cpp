#include "./loop_n_times.hpp"
#include "../../utils/unique_id.hpp"

namespace qat::ast {

LoopNTimes::LoopNTimes(Expression *_count, Block *_block, Block *_after,
                       utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), block(_block), after(_after),
      count(_count) {}

u64 LoopNTimes::new_loop_index_id(llvm::BasicBlock *bb) {
  u64 result = 0;
  bb         = &bb->getParent()->getEntryBlock();
  for (auto &inst : *bb) {
    if (llvm::isa<llvm::AllocaInst>(inst)) {
      auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&inst);
      if (alloc->getName().str().find("loop:times:index:") != String::npos) {
        result++;
      }
    } else {
      break;
    }
  }
  return result;
}

IR::Value *LoopNTimes::emit(IR::Context *ctx) {
  // TODO - Implement this
}

} // namespace qat::ast