#include "./entity.hpp"
#include "../../IR/types/region.hpp"

namespace qat::ast {

IR::PrerunValue* PrerunEntity::emit(IR::Context* ctx) {
  SHOW("PrerunEntity")
  auto* mod  = ctx->getMod();
  auto  name = identifiers.back();
  if (identifiers.size() == 1 && relative == 0) {
    if (ctx->hasActiveFunction() && ctx->getActiveFunction()->hasGenericParameter(identifiers[0].value)) {
      SHOW("PrerunEntity: Has active function and generic parameter")
      auto* genVal = ctx->getActiveFunction()->getGenericParameter(identifiers[0].value);
      if (genVal->isTyped()) {
        return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
      } else if (genVal->isPrerun()) {
        return genVal->asPrerun()->getExpression();
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    }
    if (ctx->hasActiveType()) {
      if (ctx->getActiveType()->isExpanded() && ctx->getActiveType()->asExpanded()->isGeneric()) {
        if (ctx->getActiveType()->asExpanded()->hasGenericParameter(identifiers.front().value)) {
          auto* genVal = ctx->getActiveType()->asExpanded()->getGenericParameter(identifiers.front().value);
          if (genVal->isTyped()) {
            return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
          } else if (genVal->isPrerun()) {
            return genVal->asPrerun()->getExpression();
          } else {
            ctx->Error("Invalid generic kind", fileRange);
          }
        }
      }
    }
    if (ctx->hasActiveGeneric()) {
      if (ctx->hasGenericParameterFromLastMain(identifiers[0].value)) {
        auto* genVal = ctx->getGenericParameterFromLastMain(name.value);
        if (genVal->isTyped()) {
          return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
        } else if (genVal->isPrerun()) {
          return genVal->asPrerun()->getExpression();
        } else {
          ctx->Error("Invalid generic kind", fileRange);
        }
      }
    }
    if (ctx->hasActiveFunction() && ctx->getActiveFunction()->isMemberFunction() &&
        (((IR::MemberFunction*)ctx->getActiveFunction())->getParentType()->isExpanded()) &&
        ((IR::MemberFunction*)ctx->getActiveFunction())
            ->getParentType()
            ->asExpanded()
            ->hasGenericParameter(identifiers[0].value)) {
      // FIXME - Also check generic skills
      auto* genVal = ((IR::MemberFunction*)ctx->getActiveFunction())
                         ->getParentType()
                         ->asExpanded()
                         ->getGenericParameter(identifiers[0].value);
      if (genVal->isTyped()) {
        return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
      } else if (genVal->isPrerun()) {
        return genVal->asPrerun()->getExpression();
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    }
    ctx->Error("Could not find an entity with name " + ctx->highlightError(identifiers[0].value), fileRange);
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
      if (mod->hasLib(section.value, reqInfo) ||
          mod->hasBroughtLib(section.value, ctx->getReqInfoIfDifferentModule(mod)) ||
          mod->hasAccessibleLibInImports(section.value, reqInfo).first) {
        mod = mod->getLib(section.value, reqInfo);
        mod->addMention(section.range);
      } else if (mod->hasBox(section.value, reqInfo) ||
                 mod->hasBroughtBox(section.value, ctx->getReqInfoIfDifferentModule(mod)) ||
                 mod->hasAccessibleBoxInImports(section.value, reqInfo).first) {
        mod = mod->getBox(section.value, reqInfo);
        mod->addMention(section.range);
      } else if (mod->hasBroughtModule(section.value, reqInfo) ||
                 mod->hasAccessibleBroughtModuleInImports(section.value, reqInfo).first) {
        mod = mod->getBroughtModule(section.value, reqInfo);
        mod->addMention(section.range);
      } else {
        ctx->Error("No module named " + ctx->highlightError(section.value) + " found inside the current module",
                   fileRange);
      }
    }
  }
  auto reqInfo = ctx->getAccessInfo();
  if (mod->hasTypeDef(name.value, reqInfo) ||
      mod->hasBroughtTypeDef(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
      mod->hasAccessibleTypeDefInImports(name.value, reqInfo).first) {
    auto resTypeDef = mod->getTypeDef(name.value, reqInfo);
    resTypeDef->addMention(name.range);
    return new IR::PrerunValue(IR::TypedType::get(resTypeDef));
  } else if (mod->hasCoreType(name.value, reqInfo) ||
             mod->hasBroughtCoreType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).first) {
    auto resCoreType = mod->getCoreType(name.value, reqInfo);
    resCoreType->addMention(name.range);
    return new IR::PrerunValue(IR::TypedType::get(resCoreType));
  } else if (mod->hasMixType(name.value, reqInfo) ||
             mod->hasBroughtMixType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleMixTypeInImports(name.value, reqInfo).first) {
    auto resMixType = mod->getMixType(name.value, reqInfo);
    resMixType->addMention(name.range);
    return new IR::PrerunValue(IR::TypedType::get(resMixType));
  } else if (mod->hasChoiceType(name.value, reqInfo) ||
             mod->hasBroughtChoiceType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleChoiceTypeInImports(name.value, reqInfo).first) {
    auto resChoiceType = mod->getChoiceType(name.value, reqInfo);
    resChoiceType->addMention(name.range);
    return new IR::PrerunValue(IR::TypedType::get(resChoiceType));
  } else if (mod->hasRegion(name.value, reqInfo) ||
             mod->hasBroughtRegion(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleRegionInImports(name.value, reqInfo).first) {
    auto resRegion = mod->getRegion(name.value, reqInfo);
    resRegion->addMention(name.range);
    return new IR::PrerunValue(IR::TypedType::get(resRegion));
  } else if (mod->hasGlobalEntity(name.value, reqInfo) ||
             mod->hasBroughtGlobalEntity(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGlobalEntityInImports(name.value, reqInfo).first) {
    auto* gEnt = mod->getGlobalEntity(name.value, reqInfo);
    gEnt->addMention(name.range);
    if ((!gEnt->isVariable()) && gEnt->hasInitialValue()) {
      return new IR::PrerunValue(gEnt->getInitialValue(), gEnt->getType());
    } else {
      if (gEnt->isVariable()) {
        ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) +
                       " has variability, so it cannot be used in prerun expressions",
                   name.range);
      } else {
        ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) +
                       " doesn't have an initial value that is a prerun expressions",
                   name.range);
      }
    }
    ctx->Error(ctx->highlightError(name.value) + " is a global entity.", name.range);
  } else if (mod->hasGenericCoreType(name.value, reqInfo) ||
             mod->hasBroughtGenericCoreType(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).first) {
    ctx->Error(ctx->highlightError(name.value) +
                   " is a generic core type and cannot be used as a value or type in prerun expressions",
               name.range);
  } else if (mod->hasGenericTypeDef(name.value, reqInfo) ||
             mod->hasBroughtGenericTypeDef(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericTypeDefInImports(name.value, reqInfo).first) {
    ctx->Error(ctx->highlightError(name.value) +
                   " is a generic type definition and cannot be used as a value or type in prerun expressions",
               name.range);
  } else if (mod->hasGenericFunction(name.value, reqInfo) ||
             mod->hasBroughtGenericFunction(name.value, ctx->getReqInfoIfDifferentModule(mod)) ||
             mod->hasAccessibleGenericFunctionInImports(name.value, reqInfo).first) {
    ctx->Error(ctx->highlightError(name.value) +
                   " is a generic function and cannot be used as a value or type in prerun expressions",
               fileRange);
  }
  ctx->Error("No constant entity named " + ctx->highlightError(name.value) + " found", name.range);
  return nullptr;
}

Json PrerunEntity::toJson() const {
  Vec<JsonValue> idsJs;
  for (auto const& idnt : identifiers) {
    idsJs.push_back(idnt);
  }
  return Json()._("nodeType", "constantEntity")._("identifiers", idsJs)._("fileRange", fileRange);
}

String PrerunEntity::toString() const {
  String result;
  for (usize i = 0; i < relative; i++) {
    result += "up:";
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