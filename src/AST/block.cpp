#include "./block.hpp"

namespace qat {
namespace AST {

Block::Block(std::vector<Sentence> _sentences,
             utils::FilePlacement _filePlacement)
    : sentences(_sentences), bb(nullptr), Sentence(_filePlacement) {}

llvm::BasicBlock *Block::create_bb(IR::Generator *generator) {
  if (!bb) {
    /**
     * @brief Maximum index of BasicBlock inside the function. I don't think I
     * can assume that the previous block has the maximum value, although it
     * might be good for now
     *
     */
    auto index = 0;
    auto fn = generator->builder.GetInsertBlock()->getParent();
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

llvm::Value *Block::generate(IR::Generator *generator) {
  create_bb(generator);
  generator->builder.SetInsertPoint(bb);
  llvm::Value *last = nullptr;
  for (auto sentence : sentences) {
    last = sentence.generate(generator);
  }
  return last;
}

} // namespace AST
} // namespace qat