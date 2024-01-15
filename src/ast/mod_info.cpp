#include "./mod_info.hpp"
#include "prerun/string_literal.hpp"
#include <filesystem>

namespace qat::ast {

ModInfo::ModInfo(MetaInfo _metaInfo, FileRange _fileRange) : Node(std::move(_fileRange)), metaInfo(_metaInfo) {
  SHOW("Created module info")
}

void ModInfo::define(IR::Context* ctx) {
  SHOW("ModInfo - Create Module")
  auto* mod = ctx->getMod();
  if (mod->metaInfo.has_value()) {
    ctx->Error("Meta information is already provided for module " + ctx->highlightError(mod->name.value) + " in file " +
                   ctx->highlightError(mod->filePath.string()),
               fileRange);
  }
  auto irMeta = metaInfo.toIR(ctx);
  if (irMeta.hasKey("foreign") && mod->hasMetaInfoInAnyParent("foreign")) {
    ctx->Error("The foreign ID has already been provided in one of the parent modules of this module", fileRange);
  }
  SHOW("Setting module info for " << mod->name.value << " at " << mod->filePath)
  mod->metaInfo = irMeta;
}

Json ModInfo::toJson() const { return Json()._("fileRange", fileRange); }

} // namespace qat::ast