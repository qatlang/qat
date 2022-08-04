#include "./box.hpp"
#include <nuo/json.hpp>

namespace qat::ast {

IR::Value *Box::emit(IR::Context *ctx) {
  // FIXME - Perform name checks
  // TODO - Implement this
  if (ctx) {
    std::cout << "";
  }
  return nullptr;
}

nuo::Json Box::toJson() const {
  Vec<nuo::JsonValue> mems;
  for (auto *mem : members) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "box")
      ._("members", mems)
      ._("fileRange", fileRange);
}

} // namespace qat::ast