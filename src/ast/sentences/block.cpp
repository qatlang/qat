#include "./block.hpp"

namespace qat::ast {

Block::Block(Vec<Sentence*> _sentences, FileRange _fileRange) : Sentence(_fileRange), sentences(_sentences) {}

IR::Value* Block::emit(IR::Context* ctx) {
  // TODO - Implement this
return nullptr;
}

Json Block::toJson() const {
  Vec<JsonValue> snts;
  for (auto sentence : sentences) {
    snts.push_back(sentence->toJson());
  }
  return Json()._("nodeType", "block")._("sentences", snts)._("fileRange", fileRange);
}

} // namespace qat::ast