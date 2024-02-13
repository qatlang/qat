#include "./entity.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/region.hpp"
#include "llvm/IR/GlobalVariable.h"
#include <utility>

namespace qat::ast {

IR::Value* Entity::emit(IR::Context* ctx) {
  auto* fun     = ctx->getActiveFunction();
  auto  reqInfo = ctx->getAccessInfo();
  if (ctx->hasActiveFunction()) {
    auto* mod = ctx->getMod();
    if ((names.size() == 1) && (relative == 0)) {
      auto singleName = names.front();
      if (fun->hasGenericParameter(singleName.value)) {
        auto* genVal = fun->getGenericParameter(singleName.value);
        if (genVal->isTyped()) {
          return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
        } else if (genVal->isPrerun()) {
          return genVal->asPrerun()->getExpression();
        } else {
          ctx->Error("Invalid generic kind", genVal->getRange());
        }
      } else if (ctx->hasActiveType() && (ctx->getActiveType()->isExpanded() || ctx->getActiveType()->isOpaque())) {
        auto actTy = ctx->getActiveType();
        if ((actTy->isExpanded() && actTy->asExpanded()->hasGenericParameter(singleName.value)) ||
            (actTy->isOpaque() && actTy->asOpaque()->hasGenericParameter(singleName.value))) {
          auto* genVal = actTy->isExpanded() ? actTy->asExpanded()->getGenericParameter(singleName.value)
                                             : actTy->asOpaque()->getGenericParameter(singleName.value);
          if (genVal->isTyped()) {
            return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
          } else if (genVal->isPrerun()) {
            return genVal->asPrerun()->getExpression();
          } else {
            ctx->Error("Invalid generic kind", genVal->getRange());
          }
        }
      } else if (ctx->has_active_done_skill() && (ctx->get_active_done_skill()->getType()->isExpanded() ||
                                                  ctx->get_active_done_skill()->getType()->isOpaque())) {
        auto actTy = ctx->get_active_done_skill()->getType();
        if ((actTy->isExpanded() && actTy->asExpanded()->hasGenericParameter(singleName.value)) ||
            (actTy->isOpaque() && actTy->asOpaque()->hasGenericParameter(singleName.value))) {
          auto* genVal = actTy->isExpanded() ? actTy->asExpanded()->getGenericParameter(singleName.value)
                                             : actTy->asOpaque()->getGenericParameter(singleName.value);
          if (genVal->isTyped()) {
            return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getType()));
          } else if (genVal->isPrerun()) {
            return genVal->asPrerun()->getExpression();
          } else {
            ctx->Error("Invalid generic kind", genVal->getRange());
          }
        }
      }
      SHOW("Checking block " << fun->getBlock()->getName())
      if (fun->getBlock()->hasValue(singleName.value)) {
        SHOW("Found local value: " << singleName.value)
        auto* local = fun->getBlock()->getValue(singleName.value);
        local->addMention(singleName.range);
        auto* alloca = local->getAlloca();
        if (getExpectedKind() == ExpressionKind::assignable) {
          if (local->getType()->isReference() ? local->getType()->asReference()->isSubtypeVariable()
                                              : local->isVariable()) {
            auto* val = new IR::Value(alloca, local->getType(), !local->isReference(), IR::Nature::assignable);
            val->setLocalID(local->getID());
            return val;
          } else {
            ctx->Error(ctx->highlightError(singleName.value) + " is not a variable and is not assignable",
                       names.front().range);
          }
        } else {
          auto* val = new IR::Value(alloca, local->getType(), local->isVariable(), IR::Nature::temporary);
          SHOW("Returning local value with alloca name: " << alloca->getName().str())
          val->setLocalID(local->getID());
          return val;
        }
      } else {
        SHOW("No local value with name: " << singleName.value)
        // Checking arguments
        // Currently this is unnecessary, since all arguments are allocated as
        // local values first
        auto argTypes = fun->getType()->asFunction()->getArgumentTypes();
        for (usize i = 0; i < argTypes.size(); i++) {
          if (fun->argumentNameAt(i).value == singleName.value) {
            if ((getExpectedKind() == ExpressionKind::assignable) && !argTypes.at(i)->isVariable()) {
              ctx->Error("Argument " + singleName.value +
                             " is not assignable. If the argument should be "
                             "reassigned in the function, make it var",
                         singleName.range);
            } else {
              //   mod->addMention("parameter", singleName.range, fun->argumentNameAt(i).range);
              return new IR::Value(llvm::dyn_cast<llvm::Function>(fun->getLLVM())->getArg(i), argTypes.at(i)->getType(),
                                   false, IR::Nature::pure);
            }
          }
        }
        // Checking functions
        if (mod->hasFunction(singleName.value, reqInfo) ||
            mod->hasBroughtFunction(singleName.value, ctx->getAccessInfo()) ||
            mod->hasAccessibleFunctionInImports(singleName.value, reqInfo).first) {
          return mod->getFunction(singleName.value, reqInfo);
        } else if (mod->hasGlobalEntity(singleName.value, reqInfo) ||
                   mod->hasBroughtGlobalEntity(singleName.value, ctx->getAccessInfo()) ||
                   mod->hasAccessibleGlobalEntityInImports(singleName.value, reqInfo).first) {
          auto* gEnt  = mod->getGlobalEntity(singleName.value, reqInfo);
          auto  gName = llvm::cast<llvm::GlobalVariable>(gEnt->getLLVM())->getName();
          if (!ctx->getActiveModule()->getLLVMModule()->getNamedGlobal(gName)) {
            new llvm::GlobalVariable(*ctx->getActiveModule()->getLLVMModule(), gEnt->getType()->getLLVMType(),
                                     !gEnt->isVariable(), llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, nullptr,
                                     gName, nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, std::nullopt,
                                     true);
          }
          return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
        }
      }
    } else {
      if ((relative != 0)) {
        if (ctx->getMod()->hasNthParent(relative)) {
          mod = ctx->getMod()->getNthParent(relative);
        } else {
          ctx->Error("The current scope do not have " + (relative == 1 ? "a" : std::to_string(relative)) + " parent" +
                         (relative == 1 ? "" : "s") +
                         ". Relative mentions of identifiers cannot be used here. "
                         "Please check the logic.",
                     fileRange);
        }
      }
      if (names.size() > 1) {
        for (usize i = 0; i < (names.size() - 1); i++) {
          auto split = names.at(i);
          if (relative == 0 && i == 0 && split.value == "std" && IR::StdLib::isStdLibFound()) {
            mod = IR::StdLib::stdLib;
            continue;
          }
          if (mod->hasLib(split.value, reqInfo)) {
            mod = mod->getLib(split.value, reqInfo);
            mod->addMention(split.range);
          } else if (mod->hasBox(split.value, reqInfo)) {
            mod = mod->getBox(split.value, reqInfo);
            mod->addMention(split.range);
          } else if (mod->hasBroughtModule(split.value, ctx->getAccessInfo()) ||
                     mod->hasAccessibleBroughtModuleInImports(split.value, reqInfo).first) {
            mod = mod->getBroughtModule(split.value, reqInfo);
            mod->addMention(split.range);
          } else {
            ctx->Error("No box or lib named " + ctx->highlightError(split.value) + " found inside scope ", split.range);
          }
        }
      }
    }
    auto entityName = names.back();
    if (mod->hasFunction(entityName.value, reqInfo) ||
        mod->hasBroughtFunction(entityName.value, ctx->getAccessInfo()) ||
        mod->hasAccessibleFunctionInImports(entityName.value, reqInfo).first) {
      auto* fun = mod->getFunction(entityName.value, reqInfo);
      if (!fun->isAccessible(reqInfo)) {
        ctx->Error("Function " + ctx->highlightError(fun->getFullName()) + " is not accessible here", fileRange);
      }
      fun->addMention(entityName.range);
      return fun;
    } else if (mod->hasGlobalEntity(entityName.value, reqInfo) ||
               mod->hasBroughtGlobalEntity(entityName.value, ctx->getAccessInfo()) ||
               mod->hasAccessibleGlobalEntityInImports(entityName.value, reqInfo).first) {
      auto* gEnt  = mod->getGlobalEntity(entityName.value, reqInfo);
      auto  gName = llvm::cast<llvm::GlobalVariable>(gEnt->getLLVM())->getName();
      if (!ctx->getActiveModule()->getLLVMModule()->getNamedGlobal(gName)) {
        new llvm::GlobalVariable(*ctx->getActiveModule()->getLLVMModule(), gEnt->getType()->getLLVMType(),
                                 !gEnt->isVariable(), llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, nullptr,
                                 gName, nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, std::nullopt,
                                 true);
      }
      if (!gEnt->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) + " is not accessible here", fileRange);
      }
      gEnt->addMention(entityName.range);
      return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
    } else {
      if (mod->hasLib(entityName.value, reqInfo) || mod->hasBroughtLib(entityName.value, ctx->getAccessInfo()) ||
          mod->hasAccessibleLibInImports(entityName.value, reqInfo).first) {
        ctx->Error(mod->getLib(entityName.value, reqInfo)->getFullName() +
                       " is a lib and cannot be used as a value in an expression",
                   entityName.range);
      } else if (mod->hasBox(entityName.value, reqInfo) || mod->hasBroughtBox(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleBoxInImports(entityName.value, reqInfo).first) {
        ctx->Error(mod->getBox(entityName.value, reqInfo)->getFullName() +
                       " is a box and cannot be used as a value in an expression",
                   entityName.range);
      } else if (mod->hasCoreType(entityName.value, reqInfo) ||
                 mod->hasBroughtCoreType(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleCoreTypeInImports(entityName.value, reqInfo).first) {
        auto resCoreType = mod->getCoreType(entityName.value, reqInfo);
        resCoreType->addMention(entityName.range);
        return new IR::PrerunValue(IR::TypedType::get(resCoreType));
      } else if (mod->hasMixType(entityName.value, reqInfo) ||
                 mod->hasBroughtMixType(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleMixTypeInImports(entityName.value, reqInfo).first) {
        auto resMixType = mod->getMixType(entityName.value, reqInfo);
        resMixType->addMention(entityName.range);
        return new IR::PrerunValue(IR::TypedType::get(resMixType));
      } else if (mod->hasChoiceType(entityName.value, reqInfo) ||
                 mod->hasBroughtChoiceType(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleChoiceTypeInImports(entityName.value, reqInfo).first) {
        auto* resChoiceTy = mod->getChoiceType(entityName.value, reqInfo);
        resChoiceTy->addMention(entityName.range);
        return new IR::PrerunValue(IR::TypedType::get(resChoiceTy));
      } else if (mod->hasRegion(entityName.value, reqInfo) ||
                 mod->hasBroughtRegion(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleRegionInImports(entityName.value, reqInfo).first) {
        auto* resRegion = mod->getRegion(entityName.value, reqInfo);
        resRegion->addMention(entityName.range);
        return new IR::PrerunValue(IR::TypedType::get(resRegion));
      } else if (mod->hasPrerunGlobal(entityName.value, reqInfo) ||
                 mod->hasBroughtPrerunGlobal(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessiblePrerunGlobalInImports(entityName.value, reqInfo).first) {
        auto* resPre = mod->getPrerunGlobal(entityName.value, reqInfo);
        resPre->addMention(entityName.range);
        return resPre;
      } else if (mod->hasTypeDef(entityName.value, reqInfo) ||
                 mod->hasBroughtTypeDef(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleTypeDefInImports(entityName.value, reqInfo).first) {
        ctx->Error(mod->getTypeDef(entityName.value, reqInfo)->getFullName() +
                       " is a type definition and cannot be used as a "
                       "value in an expression",
                   fileRange);
      } else if (mod->hasGenericCoreType(entityName.value, reqInfo) ||
                 mod->hasBroughtGenericCoreType(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGenericCoreTypeInImports(entityName.value, reqInfo).first) {
        ctx->Error(ctx->highlightError(entityName.value) +
                       " is a generic core type and cannot be used as a value or type",
                   entityName.range);
      } else if (mod->hasGenericTypeDef(entityName.value, reqInfo) ||
                 mod->hasBroughtGenericTypeDef(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGenericTypeDefInImports(entityName.value, reqInfo).first) {
        ctx->Error(ctx->highlightError(entityName.value) +
                       " is a generic type definition and cannot be used as a value or type",
                   entityName.range);
      } else if (mod->hasGenericFunction(entityName.value, reqInfo) ||
                 mod->hasBroughtGenericFunction(entityName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGenericFunctionInImports(entityName.value, reqInfo).first) {
        ctx->Error(ctx->highlightError(entityName.value) +
                       " is a generic function and cannot be used as a value or type",
                   fileRange);
      } else {
        ctx->Error("No entity named " + ctx->highlightError(Identifier::fullName(names).value) + " found", fileRange);
      }
    }
  } else {
    ctx->Error("No active function here", fileRange);
  }
  return nullptr;
}

Json Entity::toJson() const {
  Vec<JsonValue> namesJs;
  for (auto const& nam : names) {
    namesJs.push_back(JsonValue(nam));
  }
  return Json()._("nodeType", "entity")._("names", namesJs)._("relative", relative)._("fileRange", fileRange);
}

} // namespace qat::ast