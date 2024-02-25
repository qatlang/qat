#include "./lib.hpp"

namespace qat::ast {

void Lib::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "lib", None);
  SHOW("Creating lib")
  mod->openLib(name, fileRange.file.string(), ctx->getVisibInfo(visibSpec), ctx);
  mod->getActive()->nodes = members;
  mod->closeLib();
}

Json Lib::toJson() const {
  Vec<JsonValue> membersJsonValue;
  for (auto* mem : members) {
    membersJsonValue.emplace_back(mem->toJson());
  }
  return Json()
      ._("name", name)
      ._("nodeType", "lib")
      ._("members", membersJsonValue)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast