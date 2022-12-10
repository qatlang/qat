#include "./box.hpp"
#include "../utils/json.hpp"

namespace qat::ast {

Box::Box(Identifier _name, Vec<Node*> _members, utils::VisibilityKind _visibility, FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), members(std::move(_members)), visibility(_visibility) {}

void Box::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheck(name, "box");
  SHOW("Opening box")
  mod->openBox(name, ctx->getVisibInfo(visibility));
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