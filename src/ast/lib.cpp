#include "./lib.hpp"

namespace qat::ast {

Lib::Lib(String _name, Vec<Node *> _members,
         const utils::VisibilityInfo &_visibility, utils::FileRange _file_range)
    : name(std::move(_name)), members(std::move(_members)),
      visibility(_visibility), Node(std::move(_file_range)) {}

IR::Value *Lib::emit(IR::Context *ctx) {
  // TODO - Check imports
  if (ctx->mod->hasLib(name)) {
    ctx->Error(
        "A library named " + name +
            " exists already in this scope. Please change name of this library",
        fileRange);
  } else if (ctx->mod->hasBox(name)) {
    ctx->Error(
        "A box named " + name +
            " exists already in this scope. Please change name of this library",
        fileRange);
  }
  return nullptr;
}

nuo::Json Lib::toJson() {
  Vec<nuo::JsonValue> membersJsonValue;
  for (auto mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  return nuo::Json()
      ._("name", name)
      ._("nodeType", "lib")
      ._("members", membersJsonValue)
      ._("visibility", visibility)
      ._("fileRange", fileRange);
}

void Lib::destroy() {
  for (auto elem : members) {
    delete elem;
  }
}

} // namespace qat::ast