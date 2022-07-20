#include "./say_sentence.hpp"

namespace qat::ast {

IR::Value *Say::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json Say::toJson() const {
  std::vector<nuo::JsonValue> exps;
  for (auto exp : expressions) {
    exps.push_back(exp->toJson());
  }
  return nuo::Json()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("fileRange", fileRange);
}

} // namespace qat::ast