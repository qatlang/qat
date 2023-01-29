#include "./lib.hpp"

namespace qat::ast {

Lib::Lib(Identifier _name, Vec<Node*> _members, utils::VisibilityKind _visibility, const FileRange& _file_range)
    : Node(std::move(_file_range)), name(_name), members(std::move(_members)), visibility(_visibility) {}

void Lib::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "lib", None);
  SHOW("Creating lib")
  mod->openLib(name, fileRange.file.string(), ctx->getVisibInfo(visibility), ctx->llctx);
  mod->getActive()->nodes = members;
  mod->closeLib();
}

IR::Value* Lib::emit(IR::Context* ctx) { return nullptr; }

Json Lib::toJson() const {
  Vec<JsonValue> membersJsonValue;
  for (auto* mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  return Json()
      ._("name", name)
      ._("nodeType", "lib")
      ._("members", membersJsonValue)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast