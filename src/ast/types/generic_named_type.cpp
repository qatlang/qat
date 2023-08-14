#include "./generic_named_type.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../constants/default.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../types/const_generic.hpp"

namespace qat::ast {

GenericNamedType::GenericNamedType(u32 _relative, Vec<Identifier> _name, Vec<FillGeneric*> _types, bool _isVariable,
                                   FileRange _fileRange)
    : QatType(_isVariable, std::move(_fileRange)), relative(_relative), names(std::move(_name)),
      genericTypes(std::move(_types)) {}

IR::QatType* GenericNamedType::emit(IR::Context* ctx) {
  SHOW("Generic named type START")
  auto* mod     = ctx->getMod();
  auto* oldMod  = mod;
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
      if (mod->hasLib(split.value)) {
        mod = mod->getLib(split.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
        }
        mod->addMention(split.range);
      } else if (mod->hasBox(split.value)) {
        mod = mod->getBox(split.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Box " + ctx->highlightError(mod->getFullName()) + " is not accessible here", split.range);
        }
        mod->addMention(split.range);
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split.value) + " found inside " +
                       ctx->highlightError(mod->getFullName()) + " or any of its submodules",
                   fileRange);
      }
    }
    entityName = names.back();
  }
  auto* fun  = ctx->fn;
  auto* curr = fun ? fun->getBlock() : nullptr;
  SHOW("Got current function and block")
  if (mod->hasGenericCoreType(entityName.value) ||
      mod->hasBroughtGenericCoreType(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleGenericCoreTypeInImports(entityName.value, ctx->getAccessInfo()).first) {
    auto* genericCoreTy = mod->getGenericCoreType(entityName.value, ctx->getAccessInfo());
    if (!genericCoreTy->getVisibility().isAccessible(ctx->getAccessInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Generic core type " + ctx->highlightError(fullName.value) + " is not accessible here",
                 fullName.range);
    }
    genericCoreTy->addMention(entityName.range);
    ctx->mod = genericCoreTy->getModule();
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
        if (genericTypes.at(i)->isConst()) {
          auto* gen = genericTypes.at(i);
          if (gen->isConst() && (gen->asConst()->nodeType() == NodeType::constantDefault)) {
            ((ast::ConstantDefault*)(gen->asConst()))->setGenericAbstract(genericCoreTy->getGenericAt(i));
          } else if (genericCoreTy->getGenericAt(i)->isConst() &&
                     (genericCoreTy->getGenericAt(i)->asConst()->getType() != nullptr)) {
            gen->asConst()->setInferenceType(genericCoreTy->getGenericAt(i)->asConst()->getType());
          }
        }
        types.push_back(genericTypes.at(i)->toFill(ctx));
      }
    }
    SHOW("Filling generics")
    auto* tyRes = genericCoreTy->fillGenerics(types, ctx, fileRange);
    SHOW("Generic filled")
    ctx->fn = fun;
    if (curr) {
      curr->setActive(ctx->builder);
    }
    ctx->mod = oldMod;
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
  auto result = (isVariable() ? "var " : "") + Identifier::fullName(names).value + "'<";
  for (usize i = 0; i < genericTypes.size(); i++) {
    result += genericTypes.at(i)->toString();
    if (i != (genericTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += ">";
  return result;
}

} // namespace qat::ast