#include "./bring_entities.hpp"
#include "../IR/types/region.hpp"
#include <utility>
#include <vector>

namespace qat::ast {

BroughtGroup::BroughtGroup(u32 _relative, Vec<Identifier> _entity, FileRange _fileRange)
    : relative(_relative), entity(std::move(_entity)), fileRange(std::move(_fileRange)) {}

BroughtGroup::BroughtGroup(u32 _relative, Vec<Identifier> _entity, Vec<BroughtGroup*> _members, FileRange _fileRange)
    : relative(_relative), entity(std::move(_entity)), members(std::move(_members)), fileRange(std::move(_fileRange)) {}

void BroughtGroup::addMember(BroughtGroup* mem) { members.push_back(mem); }

void BroughtGroup::extendFileRange(FileRange end) { fileRange = FileRange(fileRange, end); }

bool BroughtGroup::hasMembers() const { return !members.empty(); }

bool BroughtGroup::isAllBrought() const { return members.empty(); }

void BroughtGroup::bring() const { isAlreadyBrought = true; }

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

BringEntities::BringEntities(Vec<BroughtGroup*> _entities, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : Node(std::move(_fileRange)), entities(std::move(_entities)), visibSpec(_visibSpec) {}

void BringEntities::handleBrings(IR::Context* ctx) const {
  auto* currentMod = ctx->getMod();
  auto  reqInfo    = ctx->getAccessInfo();

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
      if (mod->hasLib(idn.value, reqInfo) || mod->hasBroughtLib(idn.value, ctx->getReqInfoIfDifferentModule(mod)) ||
          mod->hasAccessibleLibInImports(idn.value, reqInfo).first) {
        mod = mod->getLib(idn.value, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("This lib is not accessible in the current scope", idn.range);
        }
      } else if (mod->hasBox(idn.value, reqInfo) ||
                 mod->hasBroughtBox(idn.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
    if (mod->hasLib(entName.value, reqInfo) ||
        mod->hasBroughtLib(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
    } else if (mod->hasBox(entName.value, reqInfo) ||
               mod->hasBroughtBox(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
    } else if (mod->hasBroughtModule(entName.value, ctx->getReqInfoIfDifferentModule(mod))) {
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
      if (mod->hasCoreType(entName.value, reqInfo) ||
          mod->hasBroughtCoreType(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
          mod->hasAccessibleCoreTypeInImports(entName.value, reqInfo).first) {
        auto* cTy = mod->getCoreType(entName.value, reqInfo);
        if (!cTy->isAccessible(reqInfo)) {
          ctx->Error("Core type " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringCoreType(cTy, ctx->getVisibInfo(visibSpec));
        cTy->addBroughtMention(currentMod, ent->fileRange);
        ent->bring();
      } else if (mod->hasChoiceType(entName.value, reqInfo) ||
                 mod->hasBroughtChoiceType(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtMixType(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtTypeDef(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
      } else if (mod->hasRegion(entName.value, reqInfo) ||
                 mod->hasBroughtRegion(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtFunction(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtGenericFunction(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtGenericCoreType(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
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
                 mod->hasBroughtGlobalEntity(entName.value, ctx->getReqInfoIfDifferentModule(mod)) ||
                 mod->hasAccessibleGlobalEntityInImports(entName.value, reqInfo).first) {
        auto* gEnt = mod->getGlobalEntity(entName.value, reqInfo);
        if (!gEnt->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Global entity " + ctx->highlightError(entName.value) + " is not accessible in the current scope",
                     entName.range);
        }
        currentMod->bringGlobalEntity(gEnt, ctx->getVisibInfo(visibSpec));
        gEnt->addBroughtMention(currentMod, entName.range);
        ent->bring();
      } else if (initialRunComplete) {
        ctx->Error("No module, type, function, region or global named " + ctx->highlightError(entName.value) +
                       " found in the parent scope",
                   entName.range);
      }
    }
  };
  for (auto& ent : entities) {
    bringHandler(ent, currentMod);
  }
  if (!initialRunComplete) {
    initialRunComplete = true;
  }
  // FIXME - Order of declaration can cause issues
}

IR::Value* BringEntities::emit(IR::Context* ctx) { return nullptr; }

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
    delete ent;
  }
}

} // namespace qat::ast