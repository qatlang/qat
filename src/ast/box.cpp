#include "./box.hpp"
#include <nuo/json.hpp>

namespace qat::ast {

Box::Box(String _name, Vec<Node *> _members, utils::VisibilityKind _visibility,
         utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)),
      members(std::move(_members)), visibility(_visibility) {}

void Box::createModule(IR::Context *ctx) const {
  auto *mod = ctx->getMod();
  if (mod->hasLib(name)) {
    ctx->Error(
        "A lib named " + ctx->highlightError(name) +
            " already exists in this module. Libs cannot be reopened like "
            "boxes. Please change name of this box or check the logic.",
        fileRange);
  } else if (mod->hasBroughtLib(name)) {
    ctx->Error(
        "A lib named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  } else if (mod->hasFunction(name)) {
    ctx->Error("A function named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtFunction(name)) {
    ctx->Error(
        "A function named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  } else if (mod->hasCoreType(name)) {
    ctx->Error("A core type named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtCoreType(name)) {
    ctx->Error(
        "A core type named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  } else if (mod->hasUnionType(name)) {
    ctx->Error("A union type named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtUnionType(name)) {
    ctx->Error(
        "A union type named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  } else if (mod->hasTypeDef(name)) {
    ctx->Error("A type definition named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtTypeDef(name)) {
    ctx->Error(
        "A type definition named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  } else if (mod->hasGlobalEntity(name)) {
    ctx->Error("A global entity named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtGlobalEntity(name)) {
    ctx->Error(
        "A global entity named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this box.",
        fileRange);
  }
  SHOW("Opening box")
  mod->openBox(name, ctx->getVisibInfo(visibility));
  mod->getActive()->nodes = members;
  for (auto *nod : members) {
    nod->createModule(ctx);
  }
  mod->closeBox();
}

IR::Value *Box::emit(IR::Context *ctx) { return nullptr; }

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