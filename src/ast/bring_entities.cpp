#include "./bring_entities.hpp"
#include "../IR/stdlib.hpp"
#include "../IR/types/region.hpp"
#include <utility>

namespace qat::ast {

void BroughtGroup::addMember(BroughtGroup* mem) { members.push_back(mem); }

void BroughtGroup::extendFileRange(FileRange end) { fileRange = FileRange(fileRange, end); }

bool BroughtGroup::hasMembers() const { return !members.empty(); }

bool BroughtGroup::isAllBrought() const { return members.empty(); }

void BroughtGroup::bring() const {
  isAlreadyBrought = true;
  if (entityState) {
    entityState->updateStatus(IR::EntityStatus::complete);
  }
}

Json BroughtGroup::toJson() const {
  Vec<JsonValue> entityName;
  for (auto const& idn : entity) {
    entityName.push_back(Json()._("value", idn.value)._("range", idn.range));
  }
  Vec<JsonValue> membersJson;
  for (auto const& mem : members) {
    membersJson.push_back(mem->toJson());
  }
  return Json()._("relative", relative)._("entity", entityName)._("members", membersJson)._("fileRange", fileRange);
}

void BringEntities::create_entity(IR::QatModule* currMod, IR::Context* ctx) {
  entityState = currMod->add_entity(None, IR::EntityType::bringEntity, this, IR::EmitPhase::phase_1);
  auto                                               reqInfo       = ctx->getAccessInfo();
  std::function<void(BroughtGroup*, IR::QatModule*)> createHandler = [&](BroughtGroup* ent, IR::QatModule* parentMod) {
    auto mod = parentMod;
    if (ent->relative > 0) {
      if (parentMod->hasNthParent(ent->relative)) {
        mod = parentMod->getNthParent(ent->relative);
      } else {
        ctx->Error("Current module does not have " + std::to_string(ent->relative) + " parents", ent->fileRange);
      }
    }
    for (usize i = 0; i < (ent->entity.size() - 1); i++) {
      auto const& idn = ent->entity.at(i);
      if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && IR::StdLib::isStdLibFound()) {
        mod = IR::StdLib::stdLib;
        mod->addMention(idn.range);
        continue;
      }
      if (mod->hasLib(idn.value, reqInfo) || mod->hasBroughtLib(idn.value, ctx->getAccessInfo()) ||
          mod->hasAccessibleLibInImports(idn.value, reqInfo).first) {
        mod = mod->getLib(idn.value, reqInfo);
        mod->addMention(idn.range);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This lib is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBox(idn.value, reqInfo) || mod->hasBroughtBox(idn.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleBoxInImports(idn.value, reqInfo).first) {
        mod = mod->getBox(idn.value, reqInfo);
        mod->addMention(idn.range);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This box is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBroughtModule(idn.value, reqInfo)) {
        mod = mod->getBroughtModule(idn.value, reqInfo);
        mod->addMention(idn.range);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This brought module is not accessible in the current scope", idn.range);
        }
      } else {
        ctx->Error("No box, lib or brought module named " + ctx->highlightError(idn.value) + " found", idn.range);
      }
    }
    auto entName = ent->entity.back();
    // SHOW("BringEntities name " << entName.value)
    if (mod->hasLib(entName.value, reqInfo) || mod->hasBroughtLib(entName.value, ctx->getAccessInfo()) ||
        mod->hasAccessibleLibInImports(entName.value, reqInfo).first) {
      SHOW("BringEntities: name " << entName.value)
      mod = mod->getLib(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Lib " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        ent->entityState = currMod->add_entity(entName, IR::EntityType::bringEntity, this, IR::EmitPhase::phase_1);
        entityState->addDependency(
            IR::EntityDependency{ent->entityState, IR::DependType::complete, IR::EmitPhase::phase_1});
        currMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          createHandler(mem, mod);
        }
      }
    } else if (mod->hasBox(entName.value, reqInfo) || mod->hasBroughtBox(entName.value, ctx->getAccessInfo()) ||
               mod->hasAccessibleBoxInImports(entName.value, reqInfo).first) {
      mod = mod->getBox(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Box " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        ent->entityState = currMod->add_entity(entName, IR::EntityType::bringEntity, this, IR::EmitPhase::phase_1);
        entityState->addDependency(
            IR::EntityDependency{ent->entityState, IR::DependType::complete, IR::EmitPhase::phase_1});
        currMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          createHandler(mem, mod);
        }
      }
    } else if (mod->hasBroughtModule(entName.value, ctx->getAccessInfo())) {
      mod = mod->getBroughtModule(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Brought module " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        ent->entityState = currMod->add_entity(entName, IR::EntityType::bringEntity, this, IR::EmitPhase::phase_1);
        entityState->addDependency(
            IR::EntityDependency{ent->entityState, IR::DependType::complete, IR::EmitPhase::phase_1});
        currMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          createHandler(mem, mod);
        }
      }
    } else {
      if (ent->hasMembers()) {
        ctx->Error(ctx->highlightError(entName.value) + " is not a module and hence you cannot bring its members",
                   entName.range);
      }
      ent->entityState = currMod->add_entity(entName, IR::EntityType::bringEntity, this, IR::EmitPhase::phase_1);
    }
  };
  for (auto entity : entities) {
    createHandler(entity, currMod);
  }
}

void BringEntities::update_entity_dependencies(IR::QatModule* currMod, IR::Context* ctx) {
  auto reqInfo = ctx->getAccessInfo();

  std::function<void(BroughtGroup*, IR::QatModule*)> updateHandler = [&](BroughtGroup* ent, IR::QatModule* parentMod) {
    auto mod = parentMod;
    if (ent->relative > 0) {
      if (parentMod->hasNthParent(ent->relative)) {
        mod = parentMod->getNthParent(ent->relative);
      } else {
        ctx->Error("Current module does not have " + std::to_string(ent->relative) + " parents", ent->fileRange);
      }
    }
    for (usize i = 0; i < (ent->entity.size() - 1); i++) {
      auto const& idn = ent->entity.at(i);
      if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && IR::StdLib::isStdLibFound()) {
        mod = IR::StdLib::stdLib;
        mod->addMention(idn.range);
        continue;
      }
      if (mod->hasLib(idn.value, reqInfo) || mod->hasBroughtLib(idn.value, ctx->getAccessInfo()) ||
          mod->hasAccessibleLibInImports(idn.value, reqInfo).first) {
        mod = mod->getLib(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This lib is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBox(idn.value, reqInfo) || mod->hasBroughtBox(idn.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleBoxInImports(idn.value, reqInfo).first) {
        mod = mod->getBox(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This box is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBroughtModule(idn.value, reqInfo)) {
        mod = mod->getBroughtModule(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This brought module is not accessible in the current scope", idn.range);
        }
      } else {
        ctx->Error("No box, lib or brought module named " + ctx->highlightError(idn.value) + " found", idn.range);
      }
    }
    auto entName = ent->entity.back();
    if (mod->hasLib(entName.value, reqInfo) || mod->hasBroughtLib(entName.value, ctx->getAccessInfo()) ||
        mod->hasAccessibleLibInImports(entName.value, reqInfo).first) {
      mod = mod->getLib(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Lib " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (!ent->isAllBrought()) {
        for (auto& mem : ent->members) {
          updateHandler(mem, mod);
        }
      }
    } else if (mod->hasBox(entName.value, reqInfo) || mod->hasBroughtBox(entName.value, ctx->getAccessInfo()) ||
               mod->hasAccessibleBoxInImports(entName.value, reqInfo).first) {
      mod = mod->getBox(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Box " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (!ent->isAllBrought()) {
        for (auto& mem : ent->members) {
          updateHandler(mem, mod);
        }
      }
    } else if (mod->hasBroughtModule(entName.value, ctx->getAccessInfo())) {
      mod = mod->getBroughtModule(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Brought module " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (!ent->isAllBrought()) {
        for (auto& mem : ent->members) {
          updateHandler(mem, mod);
        }
      }
    } else {
      if (ent->hasMembers()) {
        ctx->Error(ctx->highlightError(entName.value) + " is not a module and hence you cannot bring its members",
                   entName.range);
      }
      if (mod->has_entity_with_name(entName.value)) {
        ent->entityState->addDependency(
            IR::EntityDependency{mod->get_entity(entName.value), IR::DependType::complete, IR::EmitPhase::phase_1});
      } else {
        bool                                foundIt    = false;
        std::function<bool(IR::QatModule*)> modHandler = [&](IR::QatModule* module) -> bool {
          for (auto sub : module->submodules) {
            if (!sub->shouldPrefixName() && sub->has_entity_with_name(entName.value)) {
              ent->entityState->addDependency(IR::EntityDependency{sub->get_entity(entName.value),
                                                                   IR::DependType::complete, IR::EmitPhase::phase_1});
              return true;
            } else if (!sub->shouldPrefixName()) {
              if (modHandler(sub)) {
                return true;
              }
            }
          }
          for (auto bMod : module->broughtModules) {
            if (!bMod.isNamed() && bMod.get()->has_entity_with_name(entName.value)) {
              ent->entityState->addDependency(IR::EntityDependency{bMod.get()->get_entity(entName.value),
                                                                   IR::DependType::complete, IR::EmitPhase::phase_1});
              return true;
            } else if (!bMod.isNamed()) {
              if (modHandler(bMod.get())) {
                return true;
              }
            }
          }
          return false;
        };
        auto modRes = modHandler(mod);
        if (!modRes) {
          ctx->Error("No recognisable entity named " + ctx->highlightError(entName.value) +
                         " could be found in the provided parent module " + ctx->highlightError(mod->getName()) +
                         " in file " + mod->getFilePath(),
                     entName.range);
        }
      }
    }
  };
  for (auto ent : entities) {
    updateHandler(ent, currMod);
  }
}

void BringEntities::do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) { handle_brings(mod, ctx); }

void BringEntities::handle_brings(IR::QatModule* currentMod, IR::Context* ctx) const {
  auto reqInfo = ctx->getAccessInfo();

  std::function<void(BroughtGroup*, IR::QatModule*)> bringHandler = [&](BroughtGroup* ent, IR::QatModule* parentMod) {
    IR::QatModule* mod = parentMod;
    if (ent->isAlreadyBrought) {
      return;
    }
    if (ent->relative > 0) {
      if (parentMod->hasNthParent(ent->relative)) {
        mod = parentMod->getNthParent(ent->relative);
      } else {
        ctx->Error("Current module does not have " + std::to_string(ent->relative) + " parents", ent->fileRange);
      }
    }
    for (usize i = 0; i < (ent->entity.size() - 1); i++) {
      auto const& idn = ent->entity.at(i);
      if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && IR::StdLib::isStdLibFound()) {
        mod = IR::StdLib::stdLib;
        continue;
      }
      if (mod->hasLib(idn.value, reqInfo) || mod->hasBroughtLib(idn.value, ctx->getAccessInfo()) ||
          mod->hasAccessibleLibInImports(idn.value, reqInfo).first) {
        mod = mod->getLib(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This lib is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBox(idn.value, reqInfo) || mod->hasBroughtBox(idn.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleBoxInImports(idn.value, reqInfo).first) {
        mod = mod->getBox(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This box is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBroughtModule(idn.value, reqInfo)) {
        mod = mod->getBroughtModule(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This brought module is not accessible in the current scope", idn.range);
        }
      } else {
        ctx->Error("No box, lib or brought module named " + ctx->highlightError(idn.value) + " found", idn.range);
      }
    }
    auto entName = ent->entity.back();
    if (mod->hasLib(entName.value, reqInfo) || mod->hasBroughtLib(entName.value, ctx->getAccessInfo()) ||
        mod->hasAccessibleLibInImports(entName.value, reqInfo).first) {
      mod = mod->getLib(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Lib " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        currentMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          bringHandler(mem, mod);
        }
      }
    } else if (mod->hasBox(entName.value, reqInfo) || mod->hasBroughtBox(entName.value, ctx->getAccessInfo()) ||
               mod->hasAccessibleBoxInImports(entName.value, reqInfo).first) {
      mod = mod->getBox(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Box " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        currentMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          bringHandler(mem, mod);
        }
      }
    } else if (mod->hasBroughtModule(entName.value, ctx->getAccessInfo())) {
      mod = mod->getBroughtModule(entName.value, reqInfo);
      if (!mod->getVisibility().isAccessible(reqInfo)) {
        ctx->Error("Brought module " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                   entName.range);
      }
      if (ent->isAllBrought()) {
        currentMod->bringModule(mod, ctx->getVisibInfo(visibSpec));
        mod->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else {
        for (auto& mem : ent->members) {
          bringHandler(mem, mod);
        }
      }
    } else {
      if (ent->hasMembers()) {
        ctx->Error(ctx->highlightError(entName.value) + " is not a module and hence you cannot bring its members",
                   entName.range);
      }
      if (mod->hasOpaqueType(entName.value, reqInfo) ||
          mod->hasBroughtOpaqueType(entName.value, ctx->getAccessInfo()) ||
          mod->hasAccessibleCoreTypeInImports(entName.value, reqInfo).first) {
        auto* oTy = mod->getOpaqueType(entName.value, reqInfo);
        if (!oTy->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Opaque type " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringOpaqueType(oTy, ctx->getVisibInfo(visibSpec));
        oTy->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else if (mod->hasCoreType(entName.value, reqInfo) ||
                 mod->hasBroughtCoreType(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleCoreTypeInImports(entName.value, reqInfo).first) {
        SHOW("Bring entity is struct")
        auto* cTy = mod->getCoreType(entName.value, reqInfo);
        if (!cTy->isAccessible(reqInfo)) {
          ctx->Error("Core type " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringCoreType(cTy, ctx->getVisibInfo(visibSpec));
        cTy->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else if (mod->hasChoiceType(entName.value, reqInfo) ||
                 mod->hasBroughtChoiceType(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleChoiceTypeInImports(entName.value, reqInfo).first) {
        auto* chTy = mod->getChoiceType(entName.value, reqInfo);
        if (!chTy->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Choice type " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringChoiceType(chTy, ctx->getVisibInfo(visibSpec));
        chTy->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else if (mod->hasMixType(entName.value, reqInfo) ||
                 mod->hasBroughtMixType(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleMixTypeInImports(entName.value, reqInfo).first) {
        auto* mTy = mod->getMixType(entName.value, reqInfo);
        if (!mTy->isAccessible(reqInfo)) {
          ctx->Error("Mix type " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringMixType(mTy, ctx->getVisibInfo(visibSpec));
        mTy->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasTypeDef(entName.value, reqInfo) ||
                 mod->hasBroughtTypeDef(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleTypeDefInImports(entName.value, reqInfo).first) {
        auto* dTy = mod->getTypeDef(entName.value, reqInfo);
        if (!dTy->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Type definition " + ctx->highlightError(entName.value) +
                         " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringTypeDefinition(dTy, ctx->getVisibInfo(visibSpec));
        dTy->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasRegion(entName.value, reqInfo) || mod->hasBroughtRegion(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleRegionInImports(entName.value, reqInfo).first) {
        auto* rTy = mod->getRegion(entName.value, reqInfo);
        if (!rTy->isAccessible(reqInfo)) {
          ctx->Error("Region " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringRegion(rTy, ctx->getVisibInfo(visibSpec));
        rTy->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasFunction(entName.value, reqInfo) ||
                 mod->hasBroughtFunction(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleFunctionInImports(entName.value, reqInfo).first) {
        auto* otherFn = mod->getFunction(entName.value, reqInfo);
        if (!otherFn->isAccessible(reqInfo)) {
          ctx->Error("Function " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringFunction(otherFn, ctx->getVisibInfo(visibSpec));
        otherFn->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasGenericFunction(entName.value, reqInfo) ||
                 mod->hasBroughtGenericFunction(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGenericFunctionInImports(entName.value, reqInfo).first) {
        auto* gnFn = mod->getGenericFunction(entName.value, reqInfo);
        if (!gnFn->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Generic function " + ctx->highlightError(entName.value) +
                         " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringGenericFunction(gnFn, ctx->getVisibInfo(visibSpec));
        gnFn->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasGenericCoreType(entName.value, reqInfo) ||
                 mod->hasBroughtGenericCoreType(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGenericCoreTypeInImports(entName.value, reqInfo).first) {
        auto* gnCTy = mod->getGenericCoreType(entName.value, reqInfo);
        if (!gnCTy->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Generic core type " + ctx->highlightError(entName.value) +
                         " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringGenericCoreType(gnCTy, ctx->getVisibInfo(visibSpec));
        gnCTy->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasGlobalEntity(entName.value, reqInfo) ||
                 mod->hasBroughtGlobalEntity(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessibleGlobalEntityInImports(entName.value, reqInfo).first) {
        auto* gEnt = mod->getGlobalEntity(entName.value, reqInfo);
        if (!gEnt->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Global entity " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringGlobalEntity(gEnt, ctx->getVisibInfo(visibSpec));
        gEnt->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (mod->hasPrerunGlobal(entName.value, reqInfo) ||
                 mod->hasBroughtPrerunGlobal(entName.value, ctx->getAccessInfo()) ||
                 mod->hasAccessiblePrerunGlobalInImports(entName.value, reqInfo).first) {
        auto* gEnt = mod->getPrerunGlobal(entName.value, reqInfo);
        if (!gEnt->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Prerun global entity " + ctx->highlightError(entName.value) +
                         " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringPrerunGlobal(gEnt, ctx->getVisibInfo(visibSpec));
        gEnt->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (throwErrorsWhenUnfound) {
        ctx->Error("No module, type, function, region , prerun global, or global named " +
                       ctx->highlightError(entName.value) + " found in the parent scope",
                   entName.range);
      }
    }
  };
  for (auto ent : entities) {
    bringHandler(ent, currentMod);
  }
  // FIXME - Order of declaration can cause issues
}

Json BringEntities::toJson() const {
  Vec<JsonValue> entitiesJson;
  for (auto const& ent : entities) {
    entitiesJson.emplace_back(ent->toJson());
  }
  return Json()
      ._("nodeType", "bringEntities")
      ._("entities", entitiesJson)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

BringEntities::~BringEntities() {
  for (auto* ent : entities) {
    std::destroy_at(ent);
  }
}

} // namespace qat::ast