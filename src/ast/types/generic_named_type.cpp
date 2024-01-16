#include "./generic_named_type.hpp"
#include "../../IR/stdlib.hpp"
#include "../../show.hpp"
#include "../prerun/default.hpp"
#include "../types/prerun_generic.hpp"

namespace qat::ast {

IR::QatType* GenericNamedType::emit(IR::Context* ctx) {
  SHOW("Generic named type START")
  auto* mod     = ctx->getMod();
  auto  reqInfo = ctx->getAccessInfo();
  if (relative != 0) {
    if (mod->hasNthParent(relative)) {
      mod = mod->getNthParent(relative);
    }
  }
  auto entityName = names.back();
  if (names.size() > 1) {
    for (usize i = 0; i < (names.size() - 1); i++) {
      auto split = names.at(i);
      if (split.value == "std" && IR::StdLib::isStdLibFound()) {
        mod = IR::StdLib::stdLib;
        continue;
      }
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
                       ctx->highlightError(mod->getFullName()) + " or any of its submodules",
                   fileRange);
      }
    }
    entityName = names.back();
  }
  auto* fun  = ctx->getActiveFunction();
  auto* curr = fun ? fun->getBlock() : nullptr;
  if (mod->hasGenericCoreType(entityName.value, reqInfo) ||
      mod->hasBroughtGenericCoreType(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleGenericCoreTypeInImports(entityName.value, ctx->getAccessInfo()).first) {
    auto* genericCoreTy = mod->getGenericCoreType(entityName.value, ctx->getAccessInfo());
    if (!genericCoreTy->getVisibility().isAccessible(ctx->getAccessInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Generic core type " + ctx->highlightError(fullName.value) + " is not accessible here",
                 fullName.range);
    }
    SHOW("Added mention for generic")
    genericCoreTy->addMention(entityName.range);
    auto*                   oldMod = ctx->setActiveModule(genericCoreTy->getModule());
    Vec<IR::GenericToFill*> types;
    if (genericTypes.empty()) {
      SHOW("Checking if all generic abstracts have defaults")
      if (!genericCoreTy->allTypesHaveDefaults()) {
        ctx->Error(
            "Not all generic parameters in this type have a default value associated with it, and hence the type parameter list cannot be empty. Use " +
                ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
            fileRange);
      }
      SHOW("Check complete")
    } else if (genericCoreTy->getTypeCount() != genericTypes.size()) {
      ctx->Error(
          "Generic core type " + ctx->highlightError(genericCoreTy->getName().value) + " has " +
              ctx->highlightError(std::to_string(genericCoreTy->getTypeCount())) + " generic parameters. But " +
              ((genericCoreTy->getTypeCount() > genericTypes.size()) ? "only " : "") +
              ctx->highlightError(std::to_string(genericTypes.size())) +
              " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
              ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
          fileRange);
    } else {
      for (usize i = 0; i < genericTypes.size(); i++) {
        if (genericTypes.at(i)->isPrerun()) {
          auto* gen = genericTypes.at(i);
          if (gen->isPrerun() && (gen->asPrerun()->nodeType() == NodeType::PRERUN_DEFAULT)) {
            ((ast::PrerunDefault*)(gen->asPrerun()))->setGenericAbstract(genericCoreTy->getGenericAt(i));
          } else if (genericCoreTy->getGenericAt(i)->isPrerun() &&
                     (genericCoreTy->getGenericAt(i)->asPrerun()->getType() != nullptr)) {
            if (gen->asPrerun()->hasTypeInferrance()) {
              gen->asPrerun()->asTypeInferrable()->setInferenceType(
                  genericCoreTy->getGenericAt(i)->asPrerun()->getType());
            }
          }
        }
        types.push_back(genericTypes.at(i)->toFill(ctx));
      }
    }
    SHOW("Filling generics")
    auto* tyRes = genericCoreTy->fillGenerics(types, ctx, fileRange);
    SHOW("Filled generics: " << tyRes->isCoreType())
    SHOW("Generic filled: " << tyRes->toString())
    SHOW(
        "  with llvm type: " << (tyRes->getLLVMType()->isStructTy() ? tyRes->getLLVMType()->getStructName().str() : ""))
    (void)ctx->setActiveFunction(fun);
    if (curr) {
      curr->setActive(ctx->builder);
    }
    (void)ctx->setActiveModule(oldMod);
    return tyRes;
  } else if (mod->hasGenericTypeDef(entityName.value, reqInfo) ||
             mod->hasBroughtGenericTypeDef(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericTypeDefInImports(entityName.value, ctx->getAccessInfo()).first) {
    auto* genericTypeDef = mod->getGenericTypeDef(entityName.value, ctx->getAccessInfo());
    if (!genericTypeDef->getVisibility().isAccessible(ctx->getAccessInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Generic type definition " + ctx->highlightError(fullName.value) + " is not accessible here",
                 fullName.range);
    }
    genericTypeDef->addMention(entityName.range);
    auto*                   oldMod = ctx->setActiveModule(genericTypeDef->getModule());
    Vec<IR::GenericToFill*> types;
    if (genericTypes.empty()) {
      SHOW("Checking if all generic abstracts have defaults")
      if (!genericTypeDef->allTypesHaveDefaults()) {
        ctx->Error(
            "Not all generic parameters in this type have a default value associated with it, and hence the generic parameter list cannot be empty. Use " +
                ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
            fileRange);
      }
      SHOW("Check complete")
    } else if (genericTypeDef->getTypeCount() != genericTypes.size()) {
      ctx->Error(
          "Generic type definition " + ctx->highlightError(genericTypeDef->getName().value) + " has " +
              ctx->highlightError(std::to_string(genericTypeDef->getTypeCount())) + " generic parameters. But " +
              ((genericTypeDef->getTypeCount() > genericTypes.size()) ? "only " : "") +
              ctx->highlightError(std::to_string(genericTypes.size())) +
              " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
              ctx->highlightError("default") + " to use the default type or value of the generic parameter.",
          fileRange);
    } else {
      for (usize i = 0; i < genericTypes.size(); i++) {
        if (genericTypes.at(i)->isPrerun()) {
          auto* gen = genericTypes.at(i);
          if (gen->isPrerun() && (gen->asPrerun()->nodeType() == NodeType::PRERUN_DEFAULT)) {
            ((ast::PrerunDefault*)(gen->asPrerun()))->setGenericAbstract(genericTypeDef->getGenericAt(i));
          } else if (genericTypeDef->getGenericAt(i)->isPrerun() &&
                     (genericTypeDef->getGenericAt(i)->asPrerun()->getType() != nullptr)) {
            if (gen->asPrerun()->hasTypeInferrance()) {
              gen->asPrerun()->asTypeInferrable()->setInferenceType(
                  genericTypeDef->getGenericAt(i)->asPrerun()->getType());
            }
          }
        }
        types.push_back(genericTypes.at(i)->toFill(ctx));
      }
    }
    auto tyRes = genericTypeDef->fillGenerics(types, ctx, fileRange);
    (void)ctx->setActiveFunction(fun);
    if (curr) {
      curr->setActive(ctx->builder);
    }
    (void)ctx->setActiveModule(oldMod);
    return tyRes;
  } else {
    // FIXME - Support static members of generic types
    ctx->Error("No generic type named " + ctx->highlightError(Identifier::fullName(names).value) +
                   " found in the current scope",
               fileRange);
  }
  return nullptr;
}

Json GenericNamedType::toJson() const {
  Vec<JsonValue> nameJs;
  for (auto const& nam : names) {
    nameJs.push_back(JsonValue(nam));
  }
  Vec<JsonValue> typs;
  for (auto* typ : genericTypes) {
    typs.push_back(typ->toJson());
  }
  return Json()._("typeKind", "genericNamed")._("names", nameJs)._("types", typs)._("fileRange", fileRange);
}

String GenericNamedType::toString() const {
  auto result = Identifier::fullName(names).value + ":[";
  for (usize i = 0; i < genericTypes.size(); i++) {
    result += genericTypes.at(i)->toString();
    if (i != (genericTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += "]";
  return result;
}

} // namespace qat::ast