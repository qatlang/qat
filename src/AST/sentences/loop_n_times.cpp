#include "./loop_n_times.hpp"
#include "../../utils/unique_id.hpp"
#include "llvm/IR/BasicBlock.h"

namespace qat {
namespace AST {

LoopNTimes::LoopNTimes(Expression *_count, Block *_block, Block *_after,
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

IR::Value *LoopNTimes::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void LoopNTimes::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    auto loopID = "loop_" + utils::unique_id();
    file.addLoopID(loopID);
    file += ("for (std::size " + loopID + " = 0; " + loopID + " < (");
    count->emitCPP(file, isHeader);
    file += ("); ++" + loopID + ") ");
    block->emitCPP(file, isHeader);
    file.popLastLoopIndex();
    file.setOpenBlock(true);
    after->emitCPP(file, isHeader);
  }
}

} // namespace AST
} // namespace qat