#include "./block.hpp"

namespace qat::ast {

Block::Block(Vec<Sentence *> _sentences, utils::FileRange _fileRange)
    : Sentence(_fileRange), sentences(_sentences) {}

IR::Value *Block::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json Block::toJson() const {
  Vec<nuo::JsonValue> snts;
  for (auto sentence : sentences) {
    snts.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "block")
      ._("sentences", snts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast