#include "./block.hpp"

namespace qat::AST {

Block::Block(std::vector<Sentence *> _sentences,
             utils::FileRange _filePlacement)
    : Sentence(_filePlacement), sentences(_sentences) {}

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

nuo::Json Block::toJson() const {
  std::vector<nuo::JsonValue> snts;
  for (auto sentence : sentences) {
    snts.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "block")
      ._("sentences", snts)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST