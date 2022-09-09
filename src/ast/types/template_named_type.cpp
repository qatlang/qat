#include "./template_named_type.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

TemplateNamedType::TemplateNamedType(u32 _relative, String _name,
                                     Vec<ast::QatType *> _types,
                                     bool                _isVariable,
                                     utils::FileRange    _fileRange)
    : QatType(_isVariable, std::move(_fileRange)), relative(_relative),
      name(std::move(_name)), templateTypes(std::move(_types)) {}

IR::QatType *TemplateNamedType::emit(IR::Context *ctx) {
  SHOW("Template named type START")
  auto *mod     = ctx->getMod();
  auto *oldMod  = mod;
  auto  reqInfo = ctx->getReqInfo();
  if (relative != 0) {
    if (mod->hasNthParent(relative)) {
      mod = mod->getNthParent(relative);
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
  auto *curr = fun ? fun->getBlock() : nullptr;
  SHOW("Got current function and block")
  if (mod->hasTemplateCoreType(entityName) ||
      mod->hasBroughtTemplateCoreType(entityName) ||
      mod->hasAccessibleTemplateCoreTypeInImports(entityName, ctx->getReqInfo())
          .first) {
    auto *tempCoreTy = mod->getTemplateCoreType(entityName, ctx->getReqInfo());
    if (!tempCoreTy->getVisibility().isAccessible(ctx->getReqInfo())) {
      ctx->Error("Template core type " + ctx->highlightError(name) +
                     " is not accessible here",
                 fileRange);
    }
    ctx->mod = tempCoreTy->getModule();
    if (tempCoreTy->getTypeCount() != templateTypes.size()) {
      ctx->Error(
          "Template core type " + ctx->highlightError(tempCoreTy->getName()) +
              " has " +
              ctx->highlightError(std::to_string(tempCoreTy->getTypeCount())) +
              " type parameters. But " +
              ((tempCoreTy->getTypeCount() > templateTypes.size()) ? "only "
                                                                   : "") +
              ctx->highlightError(std::to_string(templateTypes.size())) +
              " types were provided",
          fileRange);
    }
    Vec<IR::QatType *> types;
    for (auto *typ : templateTypes) {
      types.push_back(typ->emit(ctx));
    }
    auto *tyRes = tempCoreTy->fillTemplates(types, ctx);
    ctx->fn     = fun;
    if (curr) {
      curr->setActive(ctx->builder);
    }
    ctx->mod = oldMod;
    return tyRes;
  } else {
    // FIXME - Support static members of template types
    ctx->Error("No template core type named " + ctx->highlightError(name) +
                   " found in the current scope",
               fileRange);
  }
  return nullptr;
}

nuo::Json TemplateNamedType::toJson() const {
  Vec<nuo::JsonValue> typs;
  for (auto *typ : templateTypes) {
    typs.push_back(typ->toJson());
  }
  return nuo::Json()
      ._("typeKind", "templateNamed")
      ._("name", name)
      ._("types", typs)
      ._("fileRange", fileRange);
}

String TemplateNamedType::toString() const {
  auto result = (isVariable() ? "var " : "") + name + ":<";
  for (usize i = 0; i < templateTypes.size(); i++) {
    result += templateTypes.at(i)->toString();
    if (i != (templateTypes.size() - 1)) {
      result += ", ";
    }
  }
  return result;
}

} // namespace qat::ast