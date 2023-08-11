#include "./entity.hpp"
#include "../../IR/types/region.hpp"

namespace qat::ast {

ConstantEntity::ConstantEntity(u32 _relative, Vec<Identifier> _ids, FileRange _range)
    : ConstantExpression(std::move(_range)), relative(_relative), identifiers(std::move(_ids)) {}

IR::ConstantValue* ConstantEntity::emit(IR::Context* ctx) {
  auto* mod  = ctx->getMod();
  auto  name = identifiers.back();
  if (identifiers.size() == 1 && relative == 0) {
    if (ctx->fn && ctx->fn->hasGenericParameter(identifiers.front().value)) {
      auto* genVal = ctx->fn->getGenericParameter(identifiers.front().value);
      if (genVal->isTyped()) {
        return new IR::ConstantValue(IR::TypedType::get(genVal->asTyped()->getType()));
      } else if (genVal->isConst()) {
        return genVal->asConst()->getExpression();
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    } else if (ctx->activeType) {
      if (ctx->activeType->hasGenericParameter(identifiers.front().value)) {
        auto* genVal = ctx->activeType->getGenericParameter(identifiers.front().value);
        if (genVal->isTyped()) {
          return new IR::ConstantValue(IR::TypedType::get(genVal->asTyped()->getType()));
        } else if (genVal->isConst()) {
          return genVal->asConst()->getExpression();
        } else {
          ctx->Error("Invalid generic kind", fileRange);
        }
      }
    }
  } else {
    auto reqInfo = ctx->getAccessInfo();
    if (relative > 0) {
      if (mod->hasNthParent(relative)) {
        mod = mod->getNthParent(relative);
      } else {
        ctx->Error("Module does not have " + ctx->highlightError(std::to_string(relative)) + " parents", fileRange);
      }
    }
    for (usize i = 0; i < (identifiers.size() - 1); i++) {
      auto section = identifiers.at(i);
      if (mod->hasLib(section.value) || mod->hasBroughtLib(section.value, ctx->getReqInfoIfDifferentModule(mod)) ||
          mod->hasAccessibleLibInImports(section.value, reqInfo).first) {
        mod = mod->getLib(section.value, reqInfo);
      } else if (mod->hasBox(section.value) ||
                 mod->hasBroughtBox(section.value, ctx->getReqInfoIfDifferentModule(mod)) ||
                 mod->hasAccessibleBoxInImports(section.value, reqInfo).first) {
        mod = mod->getBox(section.value, reqInfo);
      } else {
        ctx->Error("No lib or box found inside the module " + ctx->highlightError(mod->getFullName()) +
                       " inside file " + ctx->highlightError(mod->getParentFile()->getFilePath()),
                   fileRange);
      }
    }
  }
  auto reqInfo = ctx->getAccessInfo();
  if (mod->hasTypeDef(name.value) || mod->hasBroughtTypeDef(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleTypeDefInImports(name.value, reqInfo).first) {
    return new IR::ConstantValue(IR::TypedType::get(mod->getTypeDef(name.value, reqInfo)));
  } else if (mod->hasCoreType(name.value) ||
             mod->hasBroughtCoreType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).first) {
    return new IR::ConstantValue(IR::TypedType::get(mod->getCoreType(name.value, reqInfo)));
  } else if (mod->hasMixType(name.value) || mod->hasBroughtMixType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleMixTypeInImports(name.value, reqInfo).first) {
    return new IR::ConstantValue(IR::TypedType::get(mod->getMixType(name.value, reqInfo)));
  } else if (mod->hasChoiceType(name.value) ||
             mod->hasBroughtChoiceType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleChoiceTypeInImports(name.value, reqInfo).first) {
    return new IR::ConstantValue(IR::TypedType::get(mod->getChoiceType(name.value, reqInfo)));
  } else if (mod->hasRegion(name.value) || mod->hasBroughtRegion(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleRegionInImports(name.value, reqInfo).first) {
    return new IR::ConstantValue(IR::TypedType::get(mod->getRegion(name.value, reqInfo)));
  } else if (mod->hasGlobalEntity(name.value) ||
             mod->hasBroughtGlobalEntity(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGlobalEntityInImports(name.value, reqInfo).first) {
    auto* gEnt = mod->getGlobalEntity(name.value, reqInfo);
    if ((!gEnt->isVariable()) && gEnt->hasInitialValue()) {
      return new IR::ConstantValue(gEnt->getInitialValue(), gEnt->getType());
    } else {
      if (gEnt->isVariable()) {
        ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) +
                       " has variability, so it cannot be used in constant expressions",
                   fileRange);
      } else {
        ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) +
                       " doesn't have an initial value that is a constant expression",
                   fileRange);
      }
    }
    ctx->Error(ctx->highlightError(name.value) + " is a global entity.", name.range);
  } else if (mod->hasGenericCoreType(name.value) ||
             mod->hasBroughtGenericCoreType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).first) {
    ctx->Error(ctx->highlightError(name.value) +
                   " is a generic core type and cannot be used as a value or type in constant expressions",
               fileRange);
  } else if (mod->hasGenericFunction(name.value) ||
             mod->hasBroughtGenericFunction(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericFunctionInImports(name.value, reqInfo).first) {
    ctx->Error(ctx->highlightError(name.value) +
                   " is a generic function and cannot be used as a value in constant expressions",
               fileRange);
  }
  ctx->Error("No constant entity named " + ctx->highlightError(name.value) + " found", name.range);
  return nullptr;
}

Json ConstantEntity::toJson() const {
  Vec<JsonValue> idsJs;
  for (auto const& idnt : identifiers) {
    idsJs.push_back(idnt);
  }
  return Json()._("nodeType", "constantEntity")._("identifiers", idsJs)._("fileRange", fileRange);
}

String ConstantEntity::toString() const {
  String result;
  for (usize i = 0; i < relative; i++) {
    result += "..:";
  }
  for (usize i = 0; i < identifiers.size(); i++) {
    result += identifiers.at(i).value;
    if (i != (identifiers.size() - 1)) {
      result += ":";
    }
  }
  return result;
}

} // namespace qat::ast