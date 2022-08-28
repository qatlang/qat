#include "./lib.hpp"

namespace qat::ast {

Lib::Lib(const String &_name, Vec<Node *> _members,
         utils::VisibilityKind _visibility, const utils::FileRange &_file_range)
    : Node(std::move(_file_range)), name(_name), members(std::move(_members)),
      visibility(_visibility) {}

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

nuo::Json Lib::toJson() const {
  Vec<nuo::JsonValue> membersJsonValue;
  for (auto *mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  return nuo::Json()
      ._("name", name)
      ._("nodeType", "lib")
      ._("members", membersJsonValue)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast