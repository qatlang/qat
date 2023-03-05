#include "./named.hpp"
#include "../../IR/types/region.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

NamedType::NamedType(u32 _relative, Vec<Identifier> _names, const bool _variable, FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), relative(_relative), names(std::move(_names)) {}

IR::QatType* NamedType::emit(IR::Context* ctx) {
  auto* mod     = ctx->getMod();
  auto  reqInfo = ctx->getReqInfo();
  if (relative != 0) {
    if (ctx->getMod()->hasNthParent(relative)) {
      mod = ctx->getMod()->getNthParent(relative);
    } else {
      ctx->Error("The active scope does not have " + std::to_string(relative) + " parents", fileRange);
    }
  }
  auto entityName = names.back();
  if (names.size() == 1) {
    if ((ctx->fn && ctx->fn->hasGenericParameter(entityName.value)) ||
        (ctx->activeType && ctx->activeType->hasGenericParameter(entityName.value))) {
      IR::GenericType* genParam;
      if (ctx->fn && ctx->fn->hasGenericParameter(entityName.value)) {
        genParam = ctx->fn->getGenericParameter(entityName.value);
      } else {
        genParam = ctx->activeType->getGenericParameter(entityName.value);
      }
      if (genParam->isTyped()) {
        return genParam->asTyped()->getType();
      } else if (genParam->isConst()) {
        auto* exp = genParam->asConst()->getExpression();
        if (exp->getType()->isTyped()) {
          return exp->getType()->asTyped()->getSubType();
        } else {
          ctx->Error("Generic parameter " + ctx->highlightError(entityName.value) +
                         " is a normal const expression and hence cannot be used as a type",
                     fileRange);
        }
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    }
  }
  if (names.size() > 1) {
    for (usize i = 0; i < (names.size() - 1); i++) {
      auto split = names.at(i);
      if (mod->hasLib(split.value)) {
        mod = mod->getLib(split.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", split.range);
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
                   split.range);
      }
    }
  }
  if (mod->hasCoreType(entityName.value) ||
      mod->hasBroughtCoreType(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleCoreTypeInImports(entityName.value, reqInfo).first) {
    auto* cTy = mod->getCoreType(entityName.value, reqInfo);
    if (!cTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 entityName.range);
    }
    cTy->addMention(entityName.range);
    return cTy;
  } else if (mod->hasTypeDef(entityName.value) ||
             mod->hasBroughtTypeDef(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleTypeDefInImports(entityName.value, reqInfo).first) {
    auto* dTy = mod->getTypeDef(entityName.value, reqInfo);
    if (!dTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Type definition " + ctx->highlightError(dTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 entityName.range);
    }
    dTy->addMention(entityName.range);
    return dTy;
  } else if (mod->hasMixType(entityName.value) ||
             mod->hasBroughtMixType(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleMixTypeInImports(entityName.value, reqInfo).first) {
    auto* mTy = mod->getMixType(entityName.value, reqInfo);
    if (!mTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Mix type " + ctx->highlightError(mTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 entityName.range);
    }
    mTy->addMention(entityName.range);
    return mTy;
  } else if (mod->hasChoiceType(entityName.value) ||
             mod->hasBroughtChoiceType(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleChoiceTypeInImports(entityName.value, reqInfo).first) {
    auto* chTy = mod->getChoiceType(entityName.value, reqInfo);
    if (!chTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Choice type " + ctx->highlightError(chTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 entityName.range);
    }
    chTy->addMention(entityName.range);
    return chTy;
  } else if (mod->hasRegion(entityName.value) ||
             mod->hasBroughtRegion(entityName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleRegionInImports(entityName.value, reqInfo).first) {
    auto* reg = mod->getRegion(entityName.value, reqInfo);
    if (!reg->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Region " + ctx->highlightError(reg->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 entityName.range);
    }
    reg->addMention(entityName.range);
    return reg;
  } else {
    ctx->Error("No type named " + Identifier::fullName(names).value + " found in scope", fileRange);
  }
  return nullptr;
}

String NamedType::getName() const { return Identifier::fullName(names).value; }

u32 NamedType::getRelative() const { return relative; }

TypeKind NamedType::typeKind() const { return TypeKind::Float; }

Json NamedType::toJson() const {
  Vec<JsonValue> nameJs;
  for (auto const& nam : names) {
    nameJs.push_back(JsonValue(nam));
  }
  return Json()._("typeKind", "named")._("names", nameJs)._("isVariable", isVariable())._("fileRange", fileRange);
}

String NamedType::toString() const { return (isVariable() ? "var " : "") + Identifier::fullName(names).value; }

} // namespace qat::ast