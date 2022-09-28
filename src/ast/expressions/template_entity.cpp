#include "./template_entity.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

TemplateEntity::TemplateEntity(u32 _relative, String _name,
                               Vec<ast::QatType *> _types,
                               utils::FileRange    _fileRange)
    : Expression(std::move(_fileRange)), relative(_relative),
      name(std::move(_name)), templateTypes(std::move(_types)) {}

IR::Value *TemplateEntity::emit(IR::Context *ctx) {
  auto *mod     = ctx->getMod();
  auto *oldMod  = mod;
  auto  reqInfo = ctx->getReqInfo();
  if (relative != 0) {
    if (mod->hasNthParent(relative)) {
      mod = mod->getNthParent(relative);
    } else {
      ctx->Error("The current scope does not have " +
                     ctx->highlightError(std::to_string(relative)) +
                     " parents. Please check the logic",
                 fileRange);
    }
  }
  auto entityName = name;
  if (name.find(':') != String::npos) {
    auto splits = utils::splitString(name, ":");
    for (usize i = 0; i < (splits.size() - 1); i++) {
      auto split = splits.at(i);
      if (mod->hasLib(split)) {
        mod = mod->getLib(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
      } else if (mod->hasBox(split)) {
        mod = mod->getBox(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Box " + ctx->highlightError(mod->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split) +
                       " found inside " +
                       ctx->highlightError(mod->getFullName()),
                   fileRange);
      }
    }
    entityName = splits.back();
  }
  auto *fun  = ctx->fn;
  auto *curr = fun ? ctx->fn->getBlock() : nullptr;
  if (mod->hasTemplateFunction(entityName) ||
      mod->hasBroughtTemplateFunction(entityName) ||
      mod->hasAccessibleTemplateFunctionInImports(entityName, ctx->getReqInfo())
          .first) {
    auto *tempFn = mod->getTemplateFunction(entityName, ctx->getReqInfo());
    if (!tempFn->getVisibility().isAccessible(ctx->getReqInfo())) {
      ctx->Error("Template function " + ctx->highlightError(name) +
                     " is not accessible here",
                 fileRange);
    }
    ctx->mod = tempFn->getModule();
    if (tempFn->getTypeCount() != templateTypes.size()) {
      ctx->Error(
          "Template function " + ctx->highlightError(tempFn->getName()) +
              " has " +
              ctx->highlightError(std::to_string(tempFn->getTypeCount())) +
              " type parameters. But " +
              ((tempFn->getTypeCount() > templateTypes.size()) ? "only " : "") +
              ctx->highlightError(std::to_string(templateTypes.size())) +
              " types were provided",
          fileRange);
    }
    Vec<IR::QatType *> types;
    for (auto *typ : templateTypes) {
      types.push_back(typ->emit(ctx));
    }
    auto *fnRes = tempFn->fillTemplates(types, ctx, fileRange);
    ctx->fn     = fun;
    if (curr) {
      curr->setActive(ctx->builder);
    }
    ctx->mod = oldMod;
    return fnRes;
  } else {
    // FIXME - Support static members of template types
    ctx->Error("No template function named " + ctx->highlightError(name) +
                   " found in the current scope",
               fileRange);
  }
  return nullptr;
}

nuo::Json TemplateEntity::toJson() const {
  Vec<nuo::JsonValue> typs;
  for (auto *typ : templateTypes) {
    typs.push_back(typ->toJson());
  }
  return nuo::Json()
      ._("nodeType", "templateEntity")
      ._("name", name)
      ._("types", typs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast