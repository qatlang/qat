#include "./lib.hpp"

namespace qat::ast {

Lib::Lib(const String &_name, Vec<Node *> _members,
         utils::VisibilityKind _visibility, const utils::FileRange &_file_range)
    : Node(std::move(_file_range)), name(_name), members(std::move(_members)),
      visibility(_visibility) {}

IR::Value *Lib::emit(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if (mod->hasLib(name)) {
    ctx->Error(
        "A lib named " + ctx->highlightError(name) +
            " already exists in this module. Libs cannot be reopened like "
            "boxes. Please change name of this lib or check the logic.",
        fileRange);
  } else if (mod->hasBroughtLib(name)) {
    ctx->Error(
        "A lib named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  } else if (mod->hasBox(name)) {
    SHOW("HAS BOX WITH NAME")
    ctx->Error("A box named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "lib or check the logic.",
               fileRange);
  } else if (mod->hasBroughtBox(name)) {
    ctx->Error(
        "A box named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  } else if (mod->hasFunction(name)) {
    ctx->Error("A function named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "lib or check the logic.",
               fileRange);
  } else if (mod->hasBroughtFunction(name)) {
    ctx->Error(
        "A function named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  } else if (mod->hasCoreType(name)) {
    ctx->Error("A core type named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "lib or check the logic.",
               fileRange);
  } else if (mod->hasBroughtCoreType(name)) {
    ctx->Error(
        "A core type named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  } else if (mod->hasTypeDef(name)) {
    ctx->Error("A type definition named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "lib or check the logic.",
               fileRange);
  } else if (mod->hasBroughtTypeDef(name)) {
    ctx->Error(
        "A type definition named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  } else if (mod->hasGlobalEntity(name)) {
    ctx->Error("A global entity named " + ctx->highlightError(name) +
                   " already exists in this module. Please change name of this "
                   "lib or check the logic.",
               fileRange);
  } else if (mod->hasBroughtGlobalEntity(name)) {
    ctx->Error(
        "A global entity named " + ctx->highlightError(name) +
            " is brought into this module. Please change name of this lib.",
        fileRange);
  }
  SHOW("Creating lib")
  mod->openLib(name, fileRange.file, ctx->getVisibInfo(visibility), ctx->llctx);
  SHOW("Emitting nodes of the lib")
  for (auto *nod : members) {
    (void)nod->emit(ctx);
  }
  SHOW("Closing lib")
  mod->closeLib();
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