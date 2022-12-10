#include "./box.hpp"
#include "../utils/json.hpp"

namespace qat::ast {

Box::Box(Identifier _name, Vec<Node*> _members, utils::VisibilityKind _visibility, FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), members(std::move(_members)), visibility(_visibility) {}

void Box::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  if (mod->hasLib(name.value)) {
    ctx->Error("A lib named " + ctx->highlightError(name.value) +
                   " already exists in this module. Libs cannot be reopened like "
                   "boxes. Please change name of this box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtLib(name.value)) {
    ctx->Error("A lib named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasFunction(name.value)) {
    ctx->Error("A function named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtFunction(name.value)) {
    ctx->Error("A function named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasCoreType(name.value)) {
    ctx->Error("A core type named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtCoreType(name.value)) {
    ctx->Error("A core type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasMixType(name.value)) {
    ctx->Error("A mix type named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtMixType(name.value)) {
    ctx->Error("A mix type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasChoiceType(name.value)) {
    ctx->Error("A choice type named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtChoiceType(name.value)) {
    ctx->Error("A choice type named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasTypeDef(name.value)) {
    ctx->Error("A type definition named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtTypeDef(name.value)) {
    ctx->Error("A type definition named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  } else if (mod->hasGlobalEntity(name.value)) {
    ctx->Error("A global entity named " + ctx->highlightError(name.value) +
                   " already exists in this module. Please change name of this "
                   "box or check the logic.",
               fileRange);
  } else if (mod->hasBroughtGlobalEntity(name.value)) {
    ctx->Error("A global entity named " + ctx->highlightError(name.value) +
                   " is brought into this module. Please change name of this box.",
               fileRange);
  }
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