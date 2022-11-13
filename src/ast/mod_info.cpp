#include "./mod_info.hpp"
#include <filesystem>

namespace qat::ast {

ModInfo::ModInfo(Maybe<Pair<String, utils::FileRange>> _outputName, Maybe<KeyValue<String>> _foreignID,
                 Vec<Pair<String, utils::FileRange>> _nativeLibs, utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), foreignID(std::move(_foreignID)), outputName(std::move(_outputName)),
      linkLibs(std::move(_nativeLibs)) {}

void ModInfo::createModule(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  SHOW("Creating module info for \n    name: " << mod->getName() << "\n    full : " << mod->getFullName())
  if (mod->isModuleInfoProvided) {
    ctx->Error("Module info is already provided for module " + mod->getName(), fileRange);
  } else {
    mod->isModuleInfoProvided = true;
  }
  if (outputName.has_value()) {
    mod->moduleInfo.outputName = outputName->first;
  }
  if (foreignID.has_value()) {
    if (foreignID->value != "C++") {
      ctx->Error("This foreign module type is not supported", foreignID->valueRange);
    }
    mod->moduleInfo.foreignID = foreignID->value;
  }
  if (!linkLibs.empty()) {
    for (usize i = 0; i < linkLibs.size(); i++) {
      for (usize j = i + 1; j < linkLibs.size(); j++) {
        if (linkLibs.at(i).first == linkLibs.at(j).first) {
          ctx->Error("This native library name is repeating", linkLibs.at(j).second);
        }
      }
      mod->moduleInfo.nativeLibsToLink.push_back(linkLibs.at(i).first);
    }
  }
}

Json ModInfo::toJson() const {
  Vec<JsonValue> nativeLibsJson;
  for (auto const& nLib : linkLibs) {
    nativeLibsJson.push_back(Json()._("name", nLib.first)._("range", nLib.second));
  }
  return Json()
      ._("nodeType", "modInfo")
      ._("hasForeignID", foreignID.has_value())
      ._("foreignID",
         foreignID.has_value() ? Json()._("value", foreignID->value)._("range", foreignID->valueRange) : Json())
      ._("nativeLibs", nativeLibsJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast