#include "./template_entity.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

TemplateEntity::TemplateEntity(u32 _relative, Vec<Identifier> _names, Vec<ast::QatType*> _types, FileRange _fileRange)
    : Expression(std::move(_fileRange)), relative(_relative), names(std::move(_names)),
      templateTypes(std::move(_types)) {}

IR::Value* TemplateEntity::emit(IR::Context* ctx) {
  auto* mod     = ctx->getMod();
  auto* oldMod  = mod;
  auto  reqInfo = ctx->getReqInfo();
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
      if (mod->hasLib(split.value)) {
        mod = mod->getLib(split.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
        }
      } else if (mod->hasBox(split.value)) {
        mod = mod->getBox(split.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Box " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
        }
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split.value) + " found inside " +
                       ctx->highlightError(mod->getFullName()),
                   fileRange);
      }
    }
  }
  auto* fun  = ctx->fn;
  auto* curr = fun ? ctx->fn->getBlock() : nullptr;
  if (mod->hasTemplateFunction(entityName.value) || mod->hasBroughtTemplateFunction(entityName.value) ||
      mod->hasAccessibleTemplateFunctionInImports(entityName.value, ctx->getReqInfo()).first) {
    auto* tempFn = mod->getTemplateFunction(entityName.value, ctx->getReqInfo());
    if (!tempFn->getVisibility().isAccessible(ctx->getReqInfo())) {
      auto fullName = Identifier::fullName(names);
      ctx->Error("Template function " + ctx->highlightError(fullName.value) + " is not accessible here",
                 fullName.range);
    }
    ctx->mod = tempFn->getModule();
    if (tempFn->getTypeCount() != templateTypes.size()) {
      ctx->Error("Template function " + ctx->highlightError(tempFn->getName().value) + " has " +
                     ctx->highlightError(std::to_string(tempFn->getTypeCount())) + " type parameters. But " +
                     ((tempFn->getTypeCount() > templateTypes.size()) ? "only " : "") +
                     ctx->highlightError(std::to_string(templateTypes.size())) + " types were provided",
                 fileRange);
    }
    Vec<IR::QatType*> types;
    for (auto* typ : templateTypes) {
      types.push_back(typ->emit(ctx));
    }
    auto* fnRes = tempFn->fillTemplates(types, ctx, fileRange);
    ctx->fn     = fun;
    if (curr) {
      curr->setActive(ctx->builder);
    }
    ctx->mod = oldMod;
    return fnRes;
  } else {
    // FIXME - Support static members of template types
    auto fullName = Identifier::fullName(names);
    ctx->Error("No template function named " + ctx->highlightError(fullName.value) + " found in the current scope",
               fullName.range);
  }
  return nullptr;
}

Json TemplateEntity::toJson() const {
  Vec<JsonValue> namesJs;
  for (auto const& nam : names) {
    namesJs.push_back(JsonValue(nam));
  }
  Vec<JsonValue> typs;
  for (auto* typ : templateTypes) {
    typs.push_back(typ->toJson());
  }
  return Json()._("nodeType", "templateEntity")._("names", namesJs)._("types", typs)._("fileRange", fileRange);
}

} // namespace qat::ast