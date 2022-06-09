#include "./block.hpp"

namespace qat {
namespace AST {

Block::Block(std::vector<Sentence> _sentences,
             utils::FilePlacement _filePlacement)
    : sentences(_sentences), bb(nullptr), end_bb(nullptr),
      Sentence(_filePlacement) {}

llvm::BasicBlock *Block::create_bb(IR::Generator *generator,
                                   llvm::Function *function) {
  if (!bb) {
    /**
     * @brief Maximum index of BasicBlock inside the function. I don't think I
     * can assume that the previous block has the maximum value, although it
     * might be good for now
     *
     */
    auto index = 0;
    auto fn =
        function ? function : generator->builder.GetInsertBlock()->getParent();
    for (auto bb_i = fn->begin(); bb_i != fn->end(); ++bb_i) {
      auto bb_index = std::stoi(bb_i->getName().str());
      if (bb_index > index) {
        index = bb_index;
      }
    }
    auto name = std::to_string(index + 1);
    bb = llvm::BasicBlock::Create(generator->llvmContext, name, fn, nullptr);
  }
  return bb;
}

llvm::BasicBlock *Block::get_end_block() const { return end_bb; }

void Block::set_alloca_scope_end(llvm::LLVMContext &ctx,
                                 std::string end_block) const {
  for (auto inst = bb->getParent()->getEntryBlock().begin();
       inst != bb->getParent()->getEntryBlock().end(); inst++) {
    if (llvm::isa<llvm::AllocaInst>(*inst)) {
      auto alloc = llvm::dyn_cast<llvm::AllocaInst>(inst);
      if (llvm::dyn_cast<llvm::MDString>(
              alloc->getMetadata("origin_block")->getOperand(0))
              ->getString()
              .str() == bb->getName().str()) {
        alloc->setMetadata(
            "end_block",
            llvm::MDNode::get(ctx, llvm::MDString::get(ctx, end_block)));
      }
    }
  }
}

llvm::Value *Block::generate(IR::Generator *generator) {
  create_bb(generator);
  generator->builder.SetInsertPoint(bb);
  llvm::Value *last = nullptr;
  for (auto sentence : sentences) {
    last = sentence.generate(generator);
  }
  end_bb = generator->builder.GetInsertBlock();
  auto latest_bb = end_bb->getName().str();
  set_alloca_scope_end(generator->llvmContext, latest_bb);
  return last;
}

} // namespace AST
} // namespace qat