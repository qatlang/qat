#include "./lib.hpp"

namespace qat {
namespace AST {

Lib::Lib(std::string _name, std::vector<Node *> _members,
         utils::VisibilityInfo _visibility,
         utils::FilePlacement _file_placement)
    : name(_name), members(_members), visibility(_visibility),
      Node(_file_placement) {}

IR::Value *Lib::emit(IR::Context *ctx) {
  // TODO - Check imports
  if (ctx->mod->hasLib(name)) {
    ctx->throw_error(
        "A library named " + name +
            " exists already in this scope. Please change name of this library",
        file_placement);
  } else if (ctx->mod->hasBox(name)) {
    ctx->throw_error(
        "A box named " + name +
            " exists already in this scope. Please change name of this library",
        file_placement);
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
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat