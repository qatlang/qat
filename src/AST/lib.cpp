#include "./lib.hpp"

namespace qat::AST {

Lib::Lib(std::string _name, std::vector<Node *> _members,
         utils::VisibilityInfo _visibility, utils::FileRange _file_range)
    : name(_name), members(_members), visibility(_visibility),
      Node(_file_range) {}

IR::Value *Lib::emit(IR::Context *ctx) {
  // TODO - Check imports
  if (ctx->mod->hasLib(name)) {
    ctx->throw_error(
        "A library named " + name +
            " exists already in this scope. Please change name of this library",
        fileRange);
  } else if (ctx->mod->hasBox(name)) {
    ctx->throw_error(
        "A box named " + name +
            " exists already in this scope. Please change name of this library",
        fileRange);
  }
  return nullptr;
}

void Lib::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += ("namespace " + name + " ");
  file.addEnclosedComment("lib");
  file += " {\n";
  for (auto mem : members) {
    mem->emitCPP(file, isHeader);
  }
  file += "\n} // namespace " + name + " (lib)\n";
}

nuo::Json Lib::toJson() {
  std::vector<nuo::JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("name", name)
      ._("nodeType", "lib")
      ._("members", mems)
      ._("visibility", visibility)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST