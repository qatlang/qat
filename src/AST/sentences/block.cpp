#include "./block.hpp"

namespace qat {
namespace AST {

Block::Block(std::vector<Sentence *> _sentences,
             utils::FilePlacement _filePlacement)
    : sentences(_sentences), Sentence(_filePlacement) {}

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