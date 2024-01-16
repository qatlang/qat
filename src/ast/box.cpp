#include "./box.hpp"
#include "../utils/json.hpp"
#include "node.hpp"

namespace qat::ast {

void Box::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "box", None);
  SHOW("Opening box")
  mod->openBox(name, ctx->getVisibInfo(visibSpec), ctx);
  mod->getActive()->nodes = members;
  for (auto* nod : members) {
    nod->createModule(ctx);
  }
  mod->closeBox();
}

IR::Value* Box::emit(IR::Context* ctx) { return nullptr; }

Json Box::toJson() const {
  Vec<JsonValue> mems;
  for (auto* mem : members) {
    mems.push_back(mem->toJson());
  }
  return Json()._("nodeType", "box")._("members", mems)._("fileRange", fileRange);
}

} // namespace qat::ast