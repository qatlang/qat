#include "./entity.hpp"
#include <utility>

namespace qat::ast {

Entity::Entity(u32 _relative, Vec<Identifier> _name, FileRange _fileRange)
    : Expression(std::move(_fileRange)), names(std::move(_name)), relative(_relative) {}

IR::Value* Entity::emit(IR::Context* ctx) {
  auto* fun     = ctx->fn;
  auto  reqInfo = ctx->getReqInfo();
  if (fun) {
    auto* mod = ctx->getMod();
    if ((names.size() == 1) && (relative == 0)) {
      auto singleName = names.front().value;
      SHOW("Checking block " << fun->getBlock()->getName())
      if (fun->getBlock()->hasValue(singleName)) {
        SHOW("Found local value: " << singleName)
        auto* local  = fun->getBlock()->getValue(singleName);
        auto* alloca = local->getAlloca();
        if (getExpectedKind() == ExpressionKind::assignable) {
          if (local->getType()->isReference() ? local->getType()->asReference()->isSubtypeVariable()
                                              : local->isVariable()) {
            auto* val = new IR::Value(alloca, local->getType(), !local->isReference(), IR::Nature::assignable);
            val->setLocalID(local->getID());
            return val;
          } else {
            ctx->Error(ctx->highlightError(singleName) + " is not a variable and is not assignable",
                       names.front().range);
          }
        } else {
          auto* val = new IR::Value(alloca, local->getType(), local->isVariable(), IR::Nature::temporary);
          SHOW("Returning local value with alloca name: " << alloca->getName().str())
          val->setLocalID(local->getID());
          return val;
        }
      } else {
        SHOW("No local value with name: " << singleName)
        // Checking arguments
        // Currently this is unnecessary, since all arguments are allocated as
        // local values first
        auto argTypes = fun->getType()->asFunction()->getArgumentTypes();
        for (usize i = 0; i < argTypes.size(); i++) {
          if (fun->argumentNameAt(i).value == singleName) {
            if ((getExpectedKind() == ExpressionKind::assignable) && !argTypes.at(i)->isVariable()) {
              ctx->Error("Argument " + singleName +
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
        if (mod->hasFunction(singleName) || mod->hasBroughtFunction(singleName) ||
            mod->hasAccessibleFunctionInImports(singleName, reqInfo).first) {
          return mod->getFunction(singleName, reqInfo);
        } else if (mod->hasGlobalEntity(singleName) || mod->hasBroughtGlobalEntity(singleName) ||
                   mod->hasAccessibleGlobalEntityInImports(singleName, reqInfo).first) {
          auto* gEnt = mod->getGlobalEntity(singleName, reqInfo);
          return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
        } else if (mod->hasChoiceType(singleName) || mod->hasBroughtChoiceType(singleName) ||
                   mod->hasAccessibleChoiceTypeInImports(singleName, reqInfo).first) {
          if (canBeChoice) {
            return new IR::Value(nullptr, mod->getChoiceType(singleName, reqInfo), false, IR::Nature::pure);
          } else {
            ctx->Error(ctx->highlightError(singleName) + " is a choice type and cannnot be used as a value", fileRange);
          }
        }
        ctx->Error("No value named " + ctx->highlightError(Identifier::fullName(names).value) + " found", fileRange);
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
      auto entityName = names.back();
      if (names.size() > 1) {
        entityName = names.back();
        for (usize i = 0; i < (names.size() - 1); i++) {
          auto split = names.at(i);
          if (mod->hasLib(split.value)) {
            mod = mod->getLib(split.value, reqInfo);
            SHOW("Lib module " << mod)
            if (!mod->getVisibility().isAccessible(reqInfo)) {
              ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", split.range);
            }
          } else if (mod->hasBox(split.value)) {
            mod = mod->getBox(split.value, reqInfo);
            SHOW("Box module " << mod)
            if (!mod->getVisibility().isAccessible(reqInfo)) {
              ctx->Error("Box " + ctx->highlightError(mod->getFullName()) + " is not accessible here", split.range);
            }
          } else if (mod->hasBroughtModule(split.value) ||
                     mod->hasAccessibleBroughtModuleInImports(split.value, reqInfo).first) {
            mod = mod->getBroughtModule(split.value, reqInfo);
            SHOW("Brought module" << mod)
          } else {
            ctx->Error("No box or lib named " + ctx->highlightError(split.value) + " found inside scope ", split.range);
          }
        }
      }
      if (mod->hasFunction(entityName.value) || mod->hasBroughtFunction(entityName.value) ||
          mod->hasAccessibleFunctionInImports(entityName.value, reqInfo).first) {
        auto* fun = mod->getFunction(entityName.value, reqInfo);
        if (!fun->isAccessible(reqInfo)) {
          ctx->Error("Function " + ctx->highlightError(fun->getFullName()) + " is not accessible here", fileRange);
        }
        return fun;
      } else if (mod->hasGlobalEntity(entityName.value) || mod->hasBroughtGlobalEntity(entityName.value) ||
                 mod->hasAccessibleGlobalEntityInImports(entityName.value, reqInfo).first) {
        auto* gEnt = mod->getGlobalEntity(entityName.value, reqInfo);
        if (!gEnt->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Global entity " + ctx->highlightError(gEnt->getFullName()) + " is not accessible here",
                     fileRange);
        }
        return new IR::Value(gEnt->getLLVM(), gEnt->getType(), gEnt->isVariable(), gEnt->getNature());
      } else {
        if (mod->hasLib(entityName.value) || mod->hasBroughtLib(entityName.value) ||
            mod->hasAccessibleLibInImports(entityName.value, reqInfo).first) {
          ctx->Error(mod->getLib(entityName.value, reqInfo)->getFullName() +
                         " is a lib and cannot be used as a value in an expression",
                     fileRange);
        } else if (mod->hasBox(entityName.value) || mod->hasBroughtBox(entityName.value) ||
                   mod->hasAccessibleBoxInImports(entityName.value, reqInfo).first) {
          ctx->Error(mod->getBox(entityName.value, reqInfo)->getFullName() +
                         " is a box and cannot be used as a value in an expression",
                     fileRange);
        } else if (mod->hasCoreType(entityName.value) || mod->hasBroughtCoreType(entityName.value) ||
                   mod->hasAccessibleCoreTypeInImports(entityName.value, reqInfo).first) {
          ctx->Error(mod->getCoreType(entityName.value, reqInfo)->getFullName() +
                         " is a core type and cannot be used as a value in "
                         "an expression",
                     fileRange);
        } else if (mod->hasMixType(entityName.value) || mod->hasBroughtMixType(entityName.value) ||
                   mod->hasAccessibleMixTypeInImports(entityName.value, reqInfo).first) {
          ctx->Error(mod->getMixType(entityName.value, reqInfo)->getFullName() +
                         " is a mix type and cannot be used as a value in an "
                         "expression",
                     fileRange);
        } else if (mod->hasChoiceType(entityName.value) || mod->hasBroughtChoiceType(entityName.value) ||
                   mod->hasAccessibleChoiceTypeInImports(entityName.value, reqInfo).first) {
          auto* chTy = mod->getChoiceType(entityName.value, reqInfo);
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
        } else if (mod->hasTypeDef(entityName.value) || mod->hasBroughtTypeDef(entityName.value) ||
                   mod->hasAccessibleTypeDefInImports(entityName.value, reqInfo).first) {
          ctx->Error(mod->getTypeDef(entityName.value, reqInfo)->getFullName() +
                         " is a type definition and cannot be used as a "
                         "value in an expression",
                     fileRange);
        } else {
          ctx->Error("No entity named " + ctx->highlightError(Identifier::fullName(names).value) + " found", fileRange);
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
  Vec<JsonValue> namesJs;
  for (auto const& nam : names) {
    namesJs.push_back(JsonValue(nam));
  }
  return Json()._("nodeType", "entity")._("names", namesJs)._("relative", relative)._("fileRange", fileRange);
}

} // namespace qat::ast