#include "./generic_named_type.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

GenericNamedType::GenericNamedType(u32 _relative, Vec<Identifier> _name, Vec<ast::QatType*> _types, bool _isVariable,
                                   FileRange _fileRange)
    : QatType(_isVariable, std::move(_fileRange)), relative(_relative), names(std::move(_name)),
      templateTypes(std::move(_types)) {}

IR::QatType* GenericNamedType::emit(IR::Context* ctx) {
  SHOW("Generic named type START")
  auto* mod     = ctx->getMod();
  auto* oldMod  = mod;
  auto  reqInfo = ctx->getReqInfo();
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
  if (mod->hasTemplateCoreType(entityName.value) || mod->hasBroughtTemplateCoreType(entityName.value) ||
      mod->hasAccessibleTemplateCoreTypeInImports(entityName.value, ctx->getReqInfo()).first) {
    auto* tempCoreTy = mod->getTemplateCoreType(entityName.value, ctx->getReqInfo());
    if (!tempCoreTy->getVisibility().isAccessible(ctx->getReqInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Generic core type " + ctx->highlightError(fullName.value) + " is not accessible here",
                 fullName.range);
    }
    tempCoreTy->addMention(entityName.range);
    ctx->mod = tempCoreTy->getModule();
    Vec<IR::QatType*> types;
    if (templateTypes.empty()) {
      if (tempCoreTy->allTypesHaveDefaults()) {
        types = tempCoreTy->getDefaults(ctx);
      } else {
        ctx->Error(
            "Not all abstracted types in this generic type have a default type associated with it, and hence the type parameter list cannot be empty",
            fileRange);
      }
    } else if (tempCoreTy->getTypeCount() != templateTypes.size()) {
      ctx->Error("Generic core type " + ctx->highlightError(tempCoreTy->getName().value) + " has " +
                     ctx->highlightError(std::to_string(tempCoreTy->getTypeCount())) + " type parameters. But " +
                     ((tempCoreTy->getTypeCount() > templateTypes.size()) ? "only " : "") +
                     ctx->highlightError(std::to_string(templateTypes.size())) + " types were provided",
                 fileRange);
    } else {
      for (auto* typ : templateTypes) {
        types.push_back(typ->emit(ctx));
      }
    }
    auto* tyRes = tempCoreTy->fillTemplates(types, ctx, fileRange);
    ctx->fn     = fun;
    if (curr) {
      curr->setActive(ctx->builder);
    }
    ctx->mod = oldMod;
    return tyRes;
  } else {
    // FIXME - Support static members of template types
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
  for (auto* typ : templateTypes) {
    typs.push_back(typ->toJson());
  }
  return Json()._("typeKind", "genericNamed")._("names", nameJs)._("types", typs)._("fileRange", fileRange);
}

String GenericNamedType::toString() const {
  auto result = (isVariable() ? "var " : "") + Identifier::fullName(names).value + "'<";
  for (usize i = 0; i < templateTypes.size(); i++) {
    result += templateTypes.at(i)->toString();
    if (i != (templateTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += ">";
  return result;
}

} // namespace qat::ast