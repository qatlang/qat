#include "./mod_info.hpp"
#include "constants/string_literal.hpp"
#include <filesystem>

namespace qat::ast {

ModInfo::ModInfo(Maybe<StringLiteral*> _outputName, Maybe<KeyValue<String>> _foreignID,
                 Vec<PrerunExpression*> _nativeLibs, FileRange _fileRange)
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
    mod->moduleInfo.outputName = outputName.value()->get_value();
  }
  if (foreignID.has_value()) {
    if ((foreignID->value != "C++") && (foreignID->value == "C")) {
      ctx->Error("This foreign module type is not supported", foreignID->valueRange);
    }
    mod->moduleInfo.foreignID = foreignID->value;
  }
  if (!linkLibs.empty()) {
    for (usize i = 0; i < linkLibs.size(); i++) {
      auto* libEmit = linkLibs.at(i)->emit(ctx);
      if (libEmit->getType()->isStringSlice()) {
        auto strVal = libEmit->getType()->asStringSlice()->toPrerunGenericString(libEmit);
        if (strVal.has_value()) {
          mod->moduleInfo.nativeLibsToLink.push_back(IR::LibToLink::fromName(
              Identifier{strVal.value(), linkLibs.at(i)->fileRange}, linkLibs.at(i)->fileRange));
        } else {
          ctx->Error("Could not convert this expression to a string at compile time", linkLibs.at(i)->fileRange);
        }
      } else {
        // FIXME - Support other types
        ctx->Error("Expressions of type " + ctx->highlightError(libEmit->getType()->toString()) +
                       " is not supported as values for libraries to link",
                   linkLibs.at(i)->fileRange);
      }
    }
  }
}

Json ModInfo::toJson() const {
  Vec<JsonValue> nativeLibsJson;
  for (auto const& nLib : linkLibs) {
    nativeLibsJson.push_back(nLib->toJson());
  }
  return Json()
      ._("nodeType", "modInfo")
      ._("hasForeignID", foreignID.has_value())
      ._("foreignID", foreignID.has_value() ? Json()
                                                  ._("key", foreignID->key.value)
                                                  ._("keyRange", foreignID->key.range)
                                                  ._("value", foreignID->value)
                                                  ._("range", foreignID->valueRange)
                                            : Json())
      ._("nativeLibs", nativeLibsJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast