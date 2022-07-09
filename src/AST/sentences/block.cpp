#include "./block.hpp"

namespace qat {
namespace AST {

Block::Block(std::vector<Sentence *> _sentences,
             utils::FilePlacement _filePlacement)
    : sentences(_sentences), bb(nullptr), end_bb(nullptr),
      Sentence(_filePlacement) {}

llvm::BasicBlock *Block::create_bb(IR::Context *ctx, llvm::Function *function) {
  auto fn = function ? function : ctx->builder.GetInsertBlock()->getParent();
  if (!bb) {
    auto name = std::to_string(utils::new_block_index(fn) + 1);
    bb = llvm::BasicBlock::Create(ctx->llvmContext, name, fn, nullptr);
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

IR::Value *Block::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void Block::emitCPP(backend::cpp::File &file, bool isHeader) const {
  bool open = file.getOpenBlock();
  file.setOpenBlock(false);
  if (!open) {
    file += "{\n";
  }
  for (auto snt : sentences) {
    snt->emitCPP(file, isHeader);
  }
  if (!open) {
    file += "}\n";
  }
}

backend::JSON Block::toJSON() const {
  std::vector<backend::JSON> snts;
  for (auto sentence : sentences) {
    snts.push_back(sentence->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "block")
      ._("sentences", snts)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat