#include "./entity.hpp"
#include "../../utils/split_string.hpp"
#include <utility>

namespace qat::ast {

Entity::Entity(u32 _relative, String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), name(std::move(_name)), relative(_relative) {}

IR::Value* Entity::emit(IR::Context* ctx) {
  auto* fun     = ctx->fn;
  auto  reqInfo = ctx->getReqInfo();
  SHOW("Entity name is " << name)
  if (fun) {
    auto* mod = ctx->getMod();
    if (name.find(':') == String::npos && (relative == 0)) {
      SHOW("Checking block " << fun->getBlock()->getName())
      if (fun->getBlock()->hasAlias(name)) {
        SHOW("Entity is an alias")
        return fun->getBlock()->getAlias(name);
      } else if (fun->getBlock()->hasValue(name)) {
        SHOW("Found local value: " << name)
        auto* local  = fun->getBlock()->getValue(name);
        auto* alloca = local->getAlloca();
        if (getExpectedKind() == ExpressionKind::assignable) {
          if (local->getType()->isReference() ? local->getType()->asReference()->isSubtypeVariable()
                                              : local->isVariable()) {
            auto* val = new IR::Value(alloca, local->getType(), !local->isReference(), IR::Nature::assignable);
            val->setLocalID(local->getID());
            return val;
          } else {
            ctx->Error(ctx->highlightError(name) + " is not a variable and is not assignable", fileRange);
          }
        } else {
          auto* val = new IR::Value(alloca, local->getType(), local->isVariable(), IR::Nature::temporary);
          SHOW("Returning local value with alloca name: " << alloca->getName().str())
          val->setLocalID(local->getID());
          return val;
        }
      } else {
        SHOW("No local value with name: " << name)
        // Checking arguments
        // Currently this is unnecessary, since all arguments are allocated as
        // local values first
        auto argTypes = fun->getType()->asFunction()->getArgumentTypes();
        for (usize i = 0; i < argTypes.size(); i++) {
          if (argTypes.at(i)->getName() == name) {
            if (getExpectedKind() == ExpressionKind::assignable) {
              ctx->Error("Argument " + name +
                             " is not assignable. If the argument should be "
                             "reassigned in the function, make it var",
                         fileRange);
            } else {
              return new IR::Value(((llvm::Function*)(fun->getLLVM()))->getArg(i), argTypes.at(i)->getType(), false,
                                   IR::Nature::pure);
            }
          }
        }
        // Checking functions
        if (mod->hasFunction(name) || mod->hasBroughtFunction(name) ||
            mod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          return mod->getFunction(name, reqInfo);
        } else if (mod->hasGlobalEntity(name) || mod->hasBroughtGlobalEntity(name) ||
                   mod->hasAccessibleGlobalEntityInImports(name, reqInfo).first) {
          auto* gEnt = mod->getGlobalEntity(name, reqInfo);
          return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
        } else if (mod->hasChoiceType(name) || mod->hasBroughtChoiceType(name) ||
                   mod->hasAccessibleChoiceTypeInImports(name, reqInfo).first) {
          if (canBeChoice) {
            return new IR::Value(nullptr, mod->getChoiceType(name, reqInfo), false, IR::Nature::pure);
          } else {
            ctx->Error(ctx->highlightError(name) + " is a choice type and cannnot be used as a value", fileRange);
          }
        }
        ctx->Error("No value named " + ctx->highlightError(name) + " found", fileRange);
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
      auto entityName = name;
      if (name.find(':') != String::npos) {
        auto splits = utils::splitString(name, ":");
        entityName  = splits.back();
        for (usize i = 0; i < (splits.size() - 1); i++) {
          auto split = splits.at(i);
          if (mod->hasLib(split)) {
            mod = mod->getLib(split, reqInfo);
            SHOW("Lib module " << mod)
            if (!mod->getVisibility().isAccessible(reqInfo)) {
              ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
            }
          } else if (mod->hasBox(split)) {
            mod = mod->getBox(split, reqInfo);
            SHOW("Box module " << mod)
            if (!mod->getVisibility().isAccessible(reqInfo)) {
              ctx->Error("Box " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
            }
          } else if (mod->hasBroughtModule(split) || mod->hasAccessibleBroughtModuleInImports(split, reqInfo).first) {
            mod = mod->getBroughtModule(split, reqInfo);
            SHOW("Brought module" << mod)
          } else {
            ctx->Error("No box or lib named " + ctx->highlightError(split) + " found inside scope ", fileRange);
          }
        }
      }
      if (mod->hasFunction(entityName) || mod->hasBroughtFunction(entityName) ||
          mod->hasAccessibleFunctionInImports(entityName, reqInfo).first) {
        auto* fun = mod->getFunction(entityName, reqInfo);
        if (!fun->isAccessible(reqInfo)) {
          ctx->Error("Function " + ctx->highlightError(fun->getFullName()) + " is not accessible here", fileRange);
        }
        return fun;
      } else if (mod->hasGlobalEntity(entityName) || mod->hasBroughtGlobalEntity(entityName) ||
                 mod->hasAccessibleGlobalEntityInImports(entityName, reqInfo).first) {
        auto* gEnt = mod->getGlobalEntity(entityName, reqInfo);
        if (!gEnt->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) + " is not accessible here",
                     fileRange);
        }
        return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
      } else {
        if (mod->hasLib(entityName) || mod->hasBroughtLib(entityName) ||
            mod->hasAccessibleLibInImports(entityName, reqInfo).first) {
          ctx->Error(mod->getLib(entityName, reqInfo)->getFullName() +
                         " is a lib and cannot be used as a value in an expression",
                     fileRange);
        } else if (mod->hasBox(entityName) || mod->hasBroughtBox(entityName) ||
                   mod->hasAccessibleBoxInImports(entityName, reqInfo).first) {
          ctx->Error(mod->getBox(entityName, reqInfo)->getFullName() +
                         " is a box and cannot be used as a value in an expression",
                     fileRange);
        } else if (mod->hasCoreType(entityName) || mod->hasBroughtCoreType(entityName) ||
                   mod->hasAccessibleCoreTypeInImports(entityName, reqInfo).first) {
          ctx->Error(mod->getCoreType(entityName, reqInfo)->getFullName() +
                         " is a core type and cannot be used as a value in "
                         "an expression",
                     fileRange);
        } else if (mod->hasMixType(entityName) || mod->hasBroughtMixType(entityName) ||
                   mod->hasAccessibleMixTypeInImports(entityName, reqInfo).first) {
          ctx->Error(mod->getMixType(entityName, reqInfo)->getFullName() +
                         " is a mix type and cannot be used as a value in an "
                         "expression",
                     fileRange);
        } else if (mod->hasChoiceType(entityName) || mod->hasBroughtChoiceType(entityName) ||
                   mod->hasAccessibleChoiceTypeInImports(entityName, reqInfo).first) {
          auto* chTy = mod->getChoiceType(entityName, reqInfo);
          if (canBeChoice) {
            if (chTy->getVisibility().isAccessible(reqInfo)) {
              return new IR::Value(nullptr, chTy, false, IR::Nature::pure);
            } else {
              ctx->Error("Choice type " + ctx->highlightError(chTy->getFullName()) + " is not accessible here",
                         fileRange);
            }
          } else {
            ctx->Error(chTy->getFullName() + " is a mix type and cannot be used as a value in an "
                                             "expression",
                       fileRange);
          }
        } else if (mod->hasTypeDef(entityName) || mod->hasBroughtTypeDef(entityName) ||
                   mod->hasAccessibleTypeDefInImports(entityName, reqInfo).first) {
          ctx->Error(mod->getTypeDef(entityName, reqInfo)->getFullName() +
                         " is a type definition and cannot be used as a "
                         "value in an expression",
                     fileRange);
        } else {
          ctx->Error("No entity named " + ctx->highlightError(name) + " found", fileRange);
        }
      }
    }
  } else {
    ctx->Error("No active function here", fileRange);
  }
  return nullptr;
}

void Entity::setCanBeChoice() { canBeChoice = true; }

Json Entity::toJson() const {
  return Json()._("nodeType", "entity")._("name", name)._("relative", relative)._("fileRange", fileRange);
}

} // namespace qat::ast