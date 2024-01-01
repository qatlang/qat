#include "./generic_entity.hpp"
#include "../../utils/split_string.hpp"
#include "../constants/default.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../types/prerun_generic.hpp"

namespace qat::ast {

GenericEntity::GenericEntity(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _types, FileRange _fileRange)
    : Expression(std::move(_fileRange)), relative(_relative), names(std::move(_names)),
      genericTypes(std::move(_types)) {}

IR::Value* GenericEntity::emit(IR::Context* ctx) {
  auto* mod     = ctx->getMod();
  auto  reqInfo = ctx->getAccessInfo();
  if (relative != 0) {
    if (mod->hasNthParent(relative)) {
      mod = mod->getNthParent(relative);
    } else {
      ctx->Error("The current scope does not have " + ctx->highlightError(std::to_string(relative)) +
                     " parents. Please check the logic",
                 fileRange);
    }
  }
  auto entityName = names.back();
  if (names.size() > 1) {
    for (usize i = 0; i < (names.size() - 1); i++) {
      auto split = names.at(i);
      if (mod->hasLib(split.value, reqInfo) || mod->hasBroughtLib(split.value, reqInfo) ||
          mod->hasAccessibleLibInImports(split.value, reqInfo).first) {
        mod = mod->getLib(split.value, reqInfo);
        mod->addMention(split.range);
      } else if (mod->hasBox(split.value, reqInfo) || mod->hasBroughtBox(split.value, reqInfo) ||
                 mod->hasAccessibleBoxInImports(split.value, reqInfo).first) {
        mod = mod->getBox(split.value, reqInfo);
        mod->addMention(split.range);
      } else if (mod->hasBroughtModule(split.value, reqInfo) ||
                 mod->hasAccessibleBroughtModuleInImports(split.value, reqInfo).first) {
        mod = mod->getBroughtModule(split.value, reqInfo);
        mod->addMention(split.range);
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split.value) + " found inside " +
                       ctx->highlightError(mod->getFullName()),
                   fileRange);
      }
    }
  }
  auto* oldFn = ctx->getActiveFunction();
  auto* curr  = ctx->hasActiveFunction() ? ctx->getActiveFunction()->getBlock() : nullptr;
  if (mod->hasGenericFunction(entityName.value, reqInfo) ||
      mod->hasBroughtGenericFunction(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleGenericFunctionInImports(entityName.value, reqInfo).first) {
    auto* genericFn = mod->getGenericFunction(entityName.value, reqInfo);
    if (!genericFn->getVisibility().isAccessible(ctx->getAccessInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Generic function " + ctx->highlightError(fullName.value) + " is not accessible here", fullName.range);
    }
    auto* oldMod = ctx->setActiveModule(genericFn->getModule());
    SHOW("Got module of generic function")
    if (genericTypes.empty()) {
      SHOW("Checking if all generic abstracts have defaults")
      if (!genericFn->allTypesHaveDefaults()) {
        ctx->Error(
            "Not all generic parameters in this function have a default type associated with it, and hence the generic values can't be empty. Use " +
                ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
            fileRange);
      }
      SHOW("Check complete")
    } else if (genericFn->getTypeCount() != genericTypes.size()) {
      ctx->Error(
          "Generic function " + ctx->highlightError(genericFn->getName().value) + " has " +
              ctx->highlightError(std::to_string(genericFn->getTypeCount())) + " generic parameters. But " +
              ((genericFn->getTypeCount() > genericTypes.size()) ? "only " : "") +
              ctx->highlightError(std::to_string(genericTypes.size())) +
              " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
              ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
          fileRange);
    }
    SHOW("About to create toFill")
    Vec<IR::GenericToFill*> types;
    for (usize i = 0; i < genericTypes.size(); i++) {
      auto* gen = genericTypes.at(i);
      if (gen->isPrerun() && (gen->asPrerun()->nodeType() == NodeType::prerunDefault)) {
        SHOW("Generic is const and const generic is default expression")
        ((ast::PrerunDefault*)(genericTypes.at(i)->asPrerun()))->setGenericAbstract(genericFn->getGenericAt(i));
      } else if (genericFn->getGenericAt(i)->isPrerun() &&
                 (genericFn->getGenericAt(i)->asPrerun()->getType() != nullptr)) {
        SHOW("Generic abstract is const and has valid type")
        if (gen->asPrerun()->hasTypeInferrance()) {
          gen->asPrerun()->asTypeInferrable()->setInferenceType(genericFn->getGenericAt(i)->asPrerun()->getType());
        }
      }
      types.push_back(genericTypes.at(i)->toFill(ctx));
    }
    SHOW("Calling fillGenerics")
    auto* fnRes = genericFn->fillGenerics(types, ctx, fileRange);
    SHOW("fillGenerics completed")
    (void)ctx->setActiveFunction(oldFn);
    if (curr) {
      curr->setActive(ctx->builder);
    }
    (void)ctx->setActiveModule(oldMod);
    return fnRes;
  } else {
    // FIXME - Support static members of generic types
    auto fullName = Identifier::fullName(names);
    ctx->Error("No generic function named " + ctx->highlightError(fullName.value) + " found in the current scope",
               fullName.range);
  }
  return nullptr;
}

Json GenericEntity::toJson() const {
  Vec<JsonValue> namesJs;
  for (auto const& nam : names) {
    namesJs.push_back(JsonValue(nam));
  }
  Vec<JsonValue> typs;
  for (auto* typ : genericTypes) {
    typs.push_back(typ->toJson());
  }
  return Json()._("nodeType", "genericEntity")._("names", namesJs)._("types", typs)._("fileRange", fileRange);
}

} // namespace qat::ast