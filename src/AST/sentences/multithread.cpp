#include "./multithread.hpp"

#define MULTITHREAD_INDEX "multithread'index"
#define MULTITHREAD_RESULT "multithread:result"

namespace qat {
namespace AST {

Multithread::Multithread(Expression *_count, Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), Sentence(_filePlacement), name(std::nullopt),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), type(std::nullopt) {}

Multithread::Multithread(Expression *_count, std::string _name, QatType *_type,
                         Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), name(_name), type(_type),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), Sentence(_filePlacement) {}

std::pair<unsigned, unsigned>
Multithread::get_index_and_position(llvm::BasicBlock *entry) {
  unsigned index = 0;
  unsigned position = 0;
  for (auto inst = entry->begin(); inst != entry->end(); ++inst) {
    if (llvm::isa<llvm::AllocaInst>(&(*inst))) {
      if ((llvm::dyn_cast<llvm::AllocaInst>(&(*inst))->getName().str().find(
               "multithread:loop:index") != std::string::npos)) {
        index++;
      }
    } else {
      break;
    }
    position++;
  }
  return {index, position};
}

IR::Value *Multithread::emit(IR::Context *ctx) {
  // TODO - Implement this
}

} // namespace AST
} // namespace qat