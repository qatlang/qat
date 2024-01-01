#include "./qat_module.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/function.hpp"
#include "../ast/node.hpp"
#include "../ast/type_definition.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "brought.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "link_names.hpp"
#include "lld/Common/Driver.h"
#include "types/core_type.hpp"
#include "types/float.hpp"
#include "types/qat_type.hpp"
#include "types/region.hpp"
#include "types/void.hpp"
#include "value.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/raw_ostream.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace qat::IR {

Vec<QatModule*> QatModule::allModules{};

void QatModule::clearAll() {
  for (auto* mod : allModules) {
    delete mod;
  }
}

bool QatModule::hasFileModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasFolderModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::folder) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

QatModule* QatModule::getFileModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return mod;
      }
    }
  }
  return nullptr;
}

QatModule* QatModule::getFolderModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::folder) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return mod;
      }
    }
  }
  return nullptr;
}

QatModule::QatModule(Identifier _name, fs::path _filepath, fs::path _basePath, ModuleType _type,
                     const VisibilityInfo& _visibility, IR::Context* ctx)
    : EntityOverview("module", Json(), _name.range), name(std::move(_name)), moduleType(_type),
      filePath(std::move(_filepath)), basePath(std::move(_basePath)), visibility(_visibility) {
  llvmModule = new llvm::Module(getFullName(), ctx->llctx);
  llvmModule->setModuleIdentifier(getFullName());
  llvmModule->setSourceFileName(filePath.string());
  llvmModule->setCodeModel(llvm::CodeModel::Medium);
  llvmModule->setSDKVersion(cli::Config::get()->getVersionTuple());
  llvmModule->setTargetTriple(cli::Config::get()->getTargetTriple());
  if (ctx->dataLayout) {
    llvmModule->setDataLayout(ctx->dataLayout.value());
  }
  allModules.push_back(this);
}

QatModule::~QatModule() {
  SHOW("Deleting module :: " << name.value << " :: " << filePath)
  delete llvmModule;
  for (auto* tFn : genericFunctions) {
    delete tFn;
  }
  for (auto* tCty : genericCoreTypes) {
    delete tCty;
  }
  for (auto* gTdef : genericTypeDefinitions) {
    delete gTdef;
  }
};

QatModule* QatModule::Create(const Identifier& name, const fs::path& filepath, const fs::path& basePath,
                             ModuleType type, const VisibilityInfo& visib_info, IR::Context* ctx) {
  return new QatModule(name, filepath, basePath, type, visib_info, ctx);
}

Vec<Function*> QatModule::collectModuleInitialisers() {
  Vec<Function*> res;
  for (auto* mod : allModules) {
    if (mod->nonConstantGlobals > 0) {
      res.push_back(mod->moduleInitialiser);
    }
  }
  return res;
}

ModuleType QatModule::getModuleType() const { return moduleType; }

const VisibilityInfo& QatModule::getVisibility() const { return visibility; }

QatModule* QatModule::getActive() { // NOLINT(misc-no-recursion)
  if (active) {
    return active->getActive();
  } else {
    return this;
  }
}

QatModule* QatModule::getParentFile() { // NOLINT(misc-no-recursion)
  if ((moduleType == ModuleType::file) || rootLib) {
    return this;
  } else {
    if (parent) {
      return parent->getParentFile();
    }
  }
  return this;
}

bool QatModule::isInForeignModuleOfType(String id) const {
  return (isModuleInfoProvided && moduleInfo.foreignID.has_value() && moduleInfo.foreignID.value() == id) ||
         (parent && parent->isInForeignModuleOfType(id));
}

bool QatModule::hasNthParent(u32 n) const {
  if (n == 1) {
    return (parent != nullptr);
  } else if (n > 1) {
    return hasNthParent(n - 1);
  } else {
    return true;
  }
}

QatModule* QatModule::getNthParent(u32 n) { // NOLINT(misc-no-recursion)
  if (hasNthParent(n)) {
    if (n == 1) {
      return parent;
    } else if (n > 1) {
      return getNthParent(n - 1);
    } else {
      return this;
    }
  } else {
    return nullptr;
  }
}

void QatModule::addMember(QatModule* mod) {
  mod->parent = this;
  submodules.push_back(mod);
}

Function* QatModule::createFunction(const Identifier& name, QatType* returnType, Vec<Argument> args, bool isVariadic,
                                    const FileRange& fileRange, const VisibilityInfo& visibility,
                                    Maybe<llvm::GlobalValue::LinkageTypes> linkage, IR::Context* ctx) {
  SHOW("Creating IR function")
  auto nmUnits = getLinkNames();
  nmUnits.addUnit(LinkNameUnit(name.value, LinkUnitType::function, None, {}), None);
  auto* fun = Function::Create(this, name, nmUnits, {/* Generics */}, IR::ReturnType::get(returnType), std::move(args),
                               isVariadic, fileRange, visibility, ctx, linkage);
  SHOW("Created function")
  functions.push_back(fun);
  return fun;
}

bool QatModule::tripleIsEquivalent(const llvm::Triple& first, const llvm::Triple& second) {
  if (first == second) {
    return true;
  } else {
    return (first.getOS() == second.getOS()) && (first.getArch() == second.getArch()) &&
           (first.getEnvironment() == second.getEnvironment()) && (first.getObjectFormat() == second.getObjectFormat());
  }
}

QatModule* QatModule::CreateSubmodule(QatModule* parent, fs::path filepath, fs::path basePath, Identifier sname,
                                      ModuleType type, const VisibilityInfo& visibilityInfo, IR::Context* ctx) {
  SHOW("Creating submodule: " << sname.value)
  auto* sub = new QatModule(std::move(sname), std::move(filepath), std::move(basePath), type, visibilityInfo, ctx);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
    SHOW("Created submodule")
  }
  return sub;
}

QatModule* QatModule::CreateFileMod(QatModule* parent, fs::path filepath, fs::path basePath, Identifier fname,
                                    Vec<String> content, Vec<ast::Node*> nodes, VisibilityInfo visibilityInfo,
                                    IR::Context* ctx) {
  auto* sub =
      new QatModule(std::move(fname), std::move(filepath), std::move(basePath), ModuleType::file, visibilityInfo, ctx);
  sub->content = std::move(content);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  sub->nodes = std::move(nodes);
  return sub;
}

QatModule* QatModule::CreateRootLib(QatModule* parent, fs::path filepath, fs::path basePath, Identifier fname,
                                    Vec<String> content, Vec<ast::Node*> nodes, const VisibilityInfo& visibilityInfo,
                                    IR::Context* ctx) {
  auto* sub =
      new QatModule(std::move(fname), std::move(filepath), std::move(basePath), ModuleType::lib, visibilityInfo, ctx);
  sub->content = std::move(content);
  sub->rootLib = true;
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  sub->nodes = std::move(nodes);
  return sub;
}

void QatModule::addFilesystemBroughtMention(IR::QatModule* otherMod, const FileRange& fileRange) {
  fsBroughtMentions.push_back(Pair<IR::QatModule*, FileRange>(otherMod, fileRange));
}

Vec<Pair<QatModule*, FileRange>> const& QatModule::getFilesystemBroughtMentions() const { return fsBroughtMentions; }

void QatModule::updateOverview() {
  String moduleTyStr;
  switch (moduleType) {
    case ModuleType::box: {
      moduleTyStr = "box";
      break;
    }
    case ModuleType::file: {
      moduleTyStr = "file";
      break;
    }
    case ModuleType::folder: {
      moduleTyStr = "folder";
      break;
    }
    case ModuleType::lib: {
      moduleTyStr = "lib";
      break;
    }
  }
  Vec<JsonValue> integerBitsVal;
  Vec<JsonValue> unsignedBitsVal;
  for (auto bit : integerBitwidths) {
    integerBitsVal.push_back(bit);
  }
  for (auto bit : unsignedBitwidths) {
    unsignedBitsVal.push_back(bit);
  }
  Vec<JsonValue> fsBroughtMentionsJson;
  for (auto const& men : fsBroughtMentions) {
    fsBroughtMentionsJson.push_back(men.second);
  }
  ovInfo._("moduleID", getID())
      ._("fullName", getFullName())
      ._("isFilesystemLib", rootLib)
      ._("moduleType", moduleTyStr)
      ._("visibility", visibility)
      ._("hasModuleInitialiser", ((moduleInitialiser != nullptr) && (nonConstantGlobals > 0)))
      ._("integerBitwidths", integerBitsVal)
      ._("unsignedBitwidths", unsignedBitsVal)
      ._("filesystemBroughtMentions", fsBroughtMentionsJson);
  if (isModuleInfoProvided) {
    ovInfo._("moduleInfo", moduleInfo);
  }
}

void QatModule::outputAllOverview(Vec<JsonValue>& modulesJson, Vec<JsonValue>& functionsJson,
                                  Vec<JsonValue>& genericFunctionsJson, Vec<JsonValue>& genericCoreTypesJson,
                                  Vec<JsonValue>& coreTypesJson, Vec<JsonValue>& mixTypesJson,
                                  Vec<JsonValue>& regionJson, Vec<JsonValue>& choiceJson, Vec<JsonValue>& defsJson) {
  if (!isOverviewOutputted) {
    isOverviewOutputted = true;
    modulesJson.push_back(overviewToJson());
    for (auto* fun : functions) {
      functionsJson.push_back(fun->overviewToJson());
    }
    for (auto* fun : genericFunctions) {
      genericFunctionsJson.push_back(fun->overviewToJson());
    }
    for (auto* cTy : coreTypes) {
      coreTypesJson.push_back(cTy->overviewToJson());
    }
    for (auto* cTy : genericCoreTypes) {
      genericCoreTypesJson.push_back(cTy->overviewToJson());
    }
    for (auto* mTy : mixTypes) {
      mixTypesJson.push_back(mTy->overviewToJson());
    }
    for (auto* reg : regions) {
      regionJson.push_back(reg->overviewToJson());
    }
    for (auto* chTy : choiceTypes) {
      choiceJson.push_back(chTy->overviewToJson());
    }
    for (auto* tDef : typeDefs) {
      defsJson.push_back(tDef->overviewToJson());
    }
    for (auto* sub : submodules) {
      sub->outputAllOverview(modulesJson, functionsJson, genericFunctionsJson, genericCoreTypesJson, coreTypesJson,
                             mixTypesJson, regionJson, choiceJson, defsJson);
    }
  }
}

String QatModule::getName() const { return name.value; }

Identifier QatModule::getNameIdentifier() const { return name; }

String QatModule::getFullName() const {
  if (parent) {
    if (parent->shouldPrefixName()) {
      if (shouldPrefixName()) {
        return parent->getFullName() + ":" + name.value;
      } else {
        return parent->getFullName();
      }
    } else {
      if (shouldPrefixName()) {
        return name.value;
      } else {
        return "";
      }
    }
  } else {
    return shouldPrefixName() ? name.value : "";
  }
}

String QatModule::getWritableName() const {
  String result;
  if (parent) {
    SHOW("Writable: has parent")
    result = parent->getWritableName() + "_";
    SHOW("Writable parent result is " << result)
  }
  switch (moduleType) {
    case ModuleType::lib: {
      result += ((rootLib ? "file-" : "lib-") + name.value);
      break;
    }
    case ModuleType::box: {
      result += ("box-" + name.value);
      break;
    }
    case ModuleType::file: {
      result += ("file-" + filePath.stem().string());
      break;
    }
    case ModuleType::folder: {
      result += ("folder-" + name.value);
      break;
    }
  }
  SHOW("Writable final result is " << result)
  return result;
}

String QatModule::getFullNameWithChild(const String& child) const {
  if (shouldPrefixName()) {
    return getFullName() + ":" + child;
  } else {
    return parent ? parent->getFullNameWithChild(child) : child;
  }
}

LinkNames QatModule::getLinkNames() const {
  if (parent) {
    auto parentVal = parent->getLinkNames();
    if (shouldPrefixName()) {
      parentVal.addUnit(LinkNameUnit(name.value, moduleType == ModuleType::box ? LinkUnitType::box : LinkUnitType::lib,
                                     moduleInfo.alternativeName, {}),
                        moduleInfo.foreignID);
    }
    return parentVal;
  } else {
    if (shouldPrefixName()) {
      return LinkNames({LinkNameUnit(name.value, moduleType == ModuleType::box ? LinkUnitType::box : LinkUnitType::lib,
                                     moduleInfo.alternativeName, {})},
                       moduleInfo.foreignID, nullptr);
    } else {
      return LinkNames({}, moduleInfo.foreignID, nullptr);
    }
  }
}

bool QatModule::shouldPrefixName() const { return (moduleType == ModuleType::box || moduleType == ModuleType::lib); }

Function* QatModule::getGlobalInitialiser(IR::Context* ctx) {
  if (!moduleInitialiser) {
    moduleInitialiser = IR::Function::Create(this, Identifier("module'initialiser'" + utils::unique_id(), {filePath}),
                                             None, {/* Generics */}, IR::ReturnType::get(IR::VoidType::get(ctx->llctx)),
                                             {}, false, name.range, VisibilityInfo::pub(), ctx);
    auto* entry       = new IR::Block(moduleInitialiser, nullptr);
    entry->setActive(ctx->builder);
  }
  return moduleInitialiser;
}

void QatModule::incrementNonConstGlobalCounter() { nonConstantGlobals++; }

bool QatModule::shouldCallInitialiser() const { return nonConstantGlobals != 0; }

bool QatModule::isSubmodule() const { return parent != nullptr; }

bool QatModule::hasSubmodules() const { return !submodules.empty(); }

void QatModule::addNamedSubmodule(const Identifier& sname, const String& filename, ModuleType type,
                                  const VisibilityInfo& visib_info, IR::Context* ctx) {
  SHOW("Creating submodule: " << sname.value)
  active = CreateSubmodule(this, filename, basePath.string(), sname, type, visib_info, ctx);
}

void QatModule::closeSubmodule() { active = nullptr; }

bool QatModule::hasLib(const String& name, AccessInfo reqInfo) const {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name) &&
        (sub->getVisibility().isAccessible(reqInfo))) {
      return true;
    } else if (!sub->shouldPrefixName()) {
      if (sub->hasLib(name, reqInfo) || sub->hasBroughtLib(name, reqInfo) ||
          sub->hasAccessibleLibInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtLib(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      auto result = false;
      if (brought.isNamed()) {
        result = (brought.name.value().value == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      } else {
        result = (brought.entity->getName() == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      }
      if (result) {
        return true;
      }
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleLibInImports( // NOLINT(misc-no-recursion)
    const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name, reqInfo) || bMod->hasBroughtLib(name, reqInfo) ||
            bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule* QatModule::getLib(const String& name, const AccessInfo& reqInfo) {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name) &&
        (sub->getVisibility().isAccessible(reqInfo))) {
      return sub;
    } else {
      if (!sub->shouldPrefixName()) {
        if (sub->hasLib(name, reqInfo) || sub->hasBroughtLib(name, reqInfo) ||
            sub->hasAccessibleLibInImports(name, reqInfo).first) {
          return sub;
        }
      }
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      auto result = false;
      if (brought.isNamed()) {
        result = (brought.name.value().value == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      } else {
        result = (brought.entity->getName() == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      }
      if (result) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name, reqInfo) || bMod->hasBroughtLib(name, reqInfo) ||
            bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getLib(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openLib(const Identifier& name, const String& filename, const VisibilityInfo& visib_info,
                        IR::Context* ctx) {
  if (!hasLib(name.value, AccessInfo::GetPrivileged())) {
    addNamedSubmodule(name, filename, ModuleType::lib, visib_info, ctx);
  }
}

void QatModule::closeLib() { closeSubmodule(); }

bool QatModule::hasBox(const String& name, AccessInfo reqInfo) const {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name) &&
        sub->getVisibility().isAccessible(reqInfo)) {
      return true;
    } else if (!sub->shouldPrefixName()) {
      if (sub->hasBox(name, reqInfo) || sub->hasBroughtBox(name, reqInfo) ||
          sub->hasAccessibleBoxInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtBox(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      auto result = false;
      if (brought.isNamed()) {
        result = (brought.name.value().value == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      } else {
        result = (brought.entity->getName() == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      }
      if (result) {
        return true;
      }
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleBoxInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name, reqInfo) || bMod->hasBroughtBox(name, reqInfo) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule* QatModule::getBox(const String& name, const AccessInfo& reqInfo) {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name) &&
        sub->getVisibility().isAccessible(reqInfo)) {
      return sub;
    } else if (!sub->shouldPrefixName()) {
      if (sub->hasBox(name, reqInfo) || sub->hasBroughtBox(name, reqInfo) ||
          sub->hasAccessibleBoxInImports(name, reqInfo).first) {
        return sub->getBox(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      auto result = false;
      if (brought.isNamed()) {
        result = (brought.name.value().value == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      } else {
        result = (brought.entity->getName() == name) && brought.visibility.isAccessible(reqInfo) &&
                 brought.entity->getVisibility().isAccessible(reqInfo);
      }
      if (result) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name, reqInfo) || bMod->hasBroughtBox(name, reqInfo) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          auto resBox = bMod->getBox(name, reqInfo);
          if (resBox->getVisibility().isAccessible(reqInfo)) {
            return resBox;
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openBox(const Identifier& _name, Maybe<VisibilityInfo> visib_info, IR::Context* ctx) {
  SHOW("Opening box: " << _name.value)
  if (hasBox(_name.value, AccessInfo::GetPrivileged())) {
    for (auto* sub : submodules) {
      if (sub->moduleType == ModuleType::box) {
        if (sub->getName() == _name.value) {
          active = sub;
          break;
        }
      }
    }
  } else {
    addNamedSubmodule(_name, filePath.string(), ModuleType::box, visib_info.value(), ctx);
  }
}

void QatModule::closeBox() { closeSubmodule(); }

bool QatModule::hasBroughtModule(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto result = false;
    if (brought.isNamed()) {
      result = (brought.name.value().value == name) && brought.visibility.isAccessible(reqInfo) &&
               brought.entity->getVisibility().isAccessible(reqInfo);
    } else {
      result = (brought.entity->getName() == name) && brought.visibility.isAccessible(reqInfo) &&
               brought.entity->getVisibility().isAccessible(reqInfo);
    }
    if (result) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleBroughtModuleInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBroughtModule(name, reqInfo) || bMod->hasAccessibleBroughtModuleInImports(name, reqInfo).first) {
          if (bMod->getBroughtModule(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            SHOW("Found module in imports")
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule* QatModule::getBroughtModule(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (!brought.isNamed()) {
      if (bMod->shouldPrefixName() && (bMod->getName() == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bMod;
      } else if (!bMod->shouldPrefixName()) {
        if (bMod->hasBroughtModule(name, reqInfo) || bMod->hasAccessibleBroughtModuleInImports(name, reqInfo).first) {
          auto resMod = bMod->getBroughtModule(name, reqInfo);
          if (resMod->getVisibility().isAccessible(reqInfo)) {
            return resMod;
          }
        }
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return bMod;
    }
  }
  return nullptr;
}

void QatModule::bringModule(QatModule* other, const VisibilityInfo& _visibility, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtModules.push_back(Brought<QatModule>(bName.value(), other, _visibility));
  } else {
    broughtModules.push_back(Brought<QatModule>(other, _visibility));
  }
}

void QatModule::bringCoreType(CoreType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtCoreTypes.push_back(Brought<CoreType>(bName.value(), cTy, visib));
  } else {
    broughtCoreTypes.push_back(Brought<CoreType>(cTy, visib));
  }
}

void QatModule::bringGenericCoreType(GenericCoreType* gCTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericCoreTypes.push_back(Brought<GenericCoreType>(bName.value(), gCTy, visib));
  } else {
    broughtGenericCoreTypes.push_back(Brought<GenericCoreType>(gCTy, visib));
  }
}

void QatModule::bringMixType(MixType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtMixTypes.push_back(Brought<MixType>(bName.value(), mTy, visib));
  } else {
    broughtMixTypes.push_back(Brought<MixType>(mTy, visib));
  }
}

void QatModule::bringChoiceType(ChoiceType* chTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtChoiceTypes.push_back(Brought<ChoiceType>(bName.value(), chTy, visib));
  } else {
    broughtChoiceTypes.push_back(Brought<ChoiceType>(chTy, visib));
  }
}

void QatModule::bringTypeDefinition(DefinitionType* dTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtTypeDefs.push_back(Brought<DefinitionType>(bName.value(), dTy, visib));
  } else {
    broughtTypeDefs.push_back(Brought<DefinitionType>(dTy, visib));
  }
}

void QatModule::bringFunction(Function* fn, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtFunctions.push_back(Brought<Function>(bName.value(), fn, visib));
  } else {
    broughtFunctions.push_back(Brought<Function>(fn, visib));
  }
}

void QatModule::bringGenericFunction(GenericFunction* gFn, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericFunctions.push_back(Brought<GenericFunction>(bName.value(), gFn, visib));
  } else {
    broughtGenericFunctions.push_back(Brought<GenericFunction>(gFn, visib));
  }
}

void QatModule::bringRegion(Region* reg, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtRegions.push_back(Brought<Region>(bName.value(), reg, visib));
  } else {
    broughtRegions.push_back(Brought<Region>(reg, visib));
  }
}

void QatModule::bringGlobalEntity(GlobalEntity* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGlobalEntities.push_back(Brought<GlobalEntity>(bName.value(), gEnt, visib));
  } else {
    broughtGlobalEntities.push_back(Brought<GlobalEntity>(gEnt, visib));
  }
}

bool QatModule::hasFunction(const String& name, AccessInfo reqInfo) const {
  SHOW("Function to be checked: " << name)
  SHOW("This pointer: " << this)
  SHOW("Function count: " << functions.size())
  for (auto* function : functions) {
    SHOW("Function in module: " << function)
    if ((function->getName().value == name) && function->getVisibility().isAccessible(reqInfo)) {
      SHOW("Found function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtFunction(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleFunctionInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        SHOW("Checking brought function " << name << " in brought module " << bMod->getName() << " from file "
                                          << bMod->getFilePath())
        if (bMod->hasFunction(name, reqInfo) || bMod->hasBroughtFunction(name, reqInfo) ||
            bMod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          if (bMod->getFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

Function* QatModule::getFunction(const String& name, const AccessInfo& reqInfo) {
  for (auto* function : functions) {
    if ((function->getName().value == name) && function->getVisibility().isAccessible(reqInfo)) {
      return function;
    }
  }
  for (const auto& brought : broughtFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name, reqInfo) || bMod->hasBroughtFunction(name, reqInfo) ||
            bMod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          if (bMod->getFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getFunction(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC FUNCTION

bool QatModule::hasGenericFunction(const String& name, AccessInfo reqInfo) const {
  for (auto* function : genericFunctions) {
    if ((function->getName().value == name) && function->getVisibility().isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericFunction(name, reqInfo) || sub->hasBroughtGenericFunction(name, reqInfo) ||
          sub->hasAccessibleGenericFunctionInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtGenericFunction(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGenericFunctionInImports(const String&     name,
                                                                    const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericFunction(name, reqInfo) || bMod->hasBroughtGenericFunction(name, reqInfo) ||
            bMod->hasAccessibleGenericFunctionInImports(name, reqInfo).first) {
          if (bMod->getGenericFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericFunction* QatModule::getGenericFunction(const String& name, const AccessInfo& reqInfo) {
  for (auto* function : genericFunctions) {
    if ((function->getName().value == name) && function->getVisibility().isAccessible(reqInfo)) {
      return function;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericFunction(name, reqInfo) || sub->hasBroughtGenericFunction(name, reqInfo) ||
          sub->hasAccessibleGenericFunctionInImports(name, reqInfo).first) {
        return sub->getGenericFunction(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericFunction(name, reqInfo) || bMod->hasBroughtGenericFunction(name, reqInfo) ||
            bMod->hasAccessibleGenericFunctionInImports(name, reqInfo).first) {
          if (bMod->getGenericFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getGenericFunction(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// REGION

bool QatModule::hasRegion(const String& name, AccessInfo reqInfo) const {
  for (auto* reg : regions) {
    if ((reg->getName().value == name) && reg->isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasRegion(name, reqInfo) || sub->hasBroughtRegion(name, reqInfo) ||
          sub->hasAccessibleRegionInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtRegion(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtRegions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleRegionInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasRegion(name, reqInfo) || bMod->hasBroughtRegion(name, reqInfo) ||
                                        bMod->hasAccessibleRegionInImports(name, reqInfo).first)) {
        if (bMod->getRegion(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

Region* QatModule::getRegion(const String& name, const AccessInfo& reqInfo) const {
  for (auto* reg : regions) {
    if ((reg->getName().value == name) && reg->isAccessible(reqInfo)) {
      return reg;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasRegion(name, reqInfo) || sub->hasBroughtRegion(name, reqInfo) ||
          sub->hasAccessibleRegionInImports(name, reqInfo).first) {
        return sub->getRegion(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtRegions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasRegion(name, reqInfo) || bMod->hasBroughtRegion(name, reqInfo) ||
            bMod->hasAccessibleRegionInImports(name, reqInfo).first) {
          if (bMod->getRegion(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getRegion(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// OPAQUE TYPE

bool QatModule::hasOpaqueType(const String& name, AccessInfo reqInfo) const {
  SHOW("Opaque count: " << opaqueTypes.size())
  for (auto* typ : opaqueTypes) {
    if ((typ->getName().value == name) && typ->getVisibility().isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasOpaqueType(name, reqInfo) || sub->hasBroughtOpaqueType(name, reqInfo) ||
          sub->hasAccessibleOpaqueTypeInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtOpaqueType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtOpaqueTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleOpaqueTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasOpaqueType(name, reqInfo) || bMod->hasBroughtOpaqueType(name, reqInfo) ||
           bMod->hasAccessibleOpaqueTypeInImports(name, reqInfo).first)) {
        if (bMod->getOpaqueType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

OpaqueType* QatModule::getOpaqueType(const String& name, const AccessInfo& reqInfo) const {
  for (auto* opaqueType : opaqueTypes) {
    if ((opaqueType->getName().value == name) && opaqueType->getVisibility().isAccessible(reqInfo)) {
      return opaqueType;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasOpaqueType(name, reqInfo) || sub->hasBroughtOpaqueType(name, reqInfo) ||
          sub->hasAccessibleOpaqueTypeInImports(name, reqInfo).first) {
        return sub->getOpaqueType(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtOpaqueTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasOpaqueType(name, reqInfo) || bMod->hasBroughtOpaqueType(name, reqInfo) ||
            bMod->hasAccessibleOpaqueTypeInImports(name, reqInfo).first) {
          if (bMod->getOpaqueType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getOpaqueType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// CORE TYPE

bool QatModule::hasCoreType(const String& name, AccessInfo reqInfo) const {
  for (auto* typ : coreTypes) {
    if ((typ->getName().value == name) && typ->isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasCoreType(name, reqInfo) || sub->hasBroughtCoreType(name, reqInfo) ||
          sub->hasAccessibleCoreTypeInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtCoreType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleCoreTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      SHOW("Brought module: " << bMod->getID() << " name: " << bMod->getFullName())
      if (!bMod->shouldPrefixName() && (bMod->hasCoreType(name, reqInfo) || bMod->hasBroughtCoreType(name, reqInfo) ||
                                        bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first)) {
        if (bMod->getCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

CoreType* QatModule::getCoreType(const String& name, const AccessInfo& reqInfo) const {
  for (auto* coreType : coreTypes) {
    if ((coreType->getName().value == name) && coreType->isAccessible(reqInfo)) {
      return coreType;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasCoreType(name, reqInfo) || sub->hasBroughtCoreType(name, reqInfo) ||
          sub->hasAccessibleCoreTypeInImports(name, reqInfo).first) {
        return sub->getCoreType(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasCoreType(name, reqInfo) || bMod->hasBroughtCoreType(name, reqInfo) ||
            bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getCoreType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// MIX TYPE

bool QatModule::hasMixType(const String& name, AccessInfo reqInfo) const {
  SHOW("MixType count: " << mixTypes.size())
  for (auto* typ : mixTypes) {
    if ((typ->getName().value == name) && typ->isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasMixType(name, reqInfo) || sub->hasBroughtMixType(name, reqInfo) ||
          sub->hasAccessibleMixTypeInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtMixType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtMixTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleMixTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasMixType(name, reqInfo) || bMod->hasBroughtMixType(name, reqInfo) ||
                                        bMod->hasAccessibleMixTypeInImports(name, reqInfo).first)) {
        if (bMod->getMixType(name, reqInfo)->isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

MixType* QatModule::getMixType(const String& name, const AccessInfo& reqInfo) const {
  for (auto* mixTy : mixTypes) {
    if ((mixTy->getName().value == name) && mixTy->isAccessible(reqInfo)) {
      return mixTy;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasMixType(name, reqInfo) || sub->hasBroughtMixType(name, reqInfo) ||
          sub->hasAccessibleMixTypeInImports(name, reqInfo).first) {
        return sub->getMixType(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtMixTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasMixType(name, reqInfo) || bMod->hasBroughtMixType(name, reqInfo) ||
            bMod->hasAccessibleMixTypeInImports(name, reqInfo).first) {
          if (bMod->getMixType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getMixType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// CHOICE TYPE

bool QatModule::hasChoiceType(const String& name, AccessInfo reqInfo) const {
  SHOW("ChoiceType count: " << mixTypes.size())
  for (auto* typ : choiceTypes) {
    if ((typ->getName().value == name) && typ->getVisibility().isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasChoiceType(name, reqInfo) || sub->hasBroughtChoiceType(name, reqInfo) ||
          sub->hasAccessibleChoiceTypeInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtChoiceType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtChoiceTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleChoiceTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasChoiceType(name, reqInfo) || bMod->hasBroughtChoiceType(name, reqInfo) ||
           bMod->hasAccessibleChoiceTypeInImports(name, reqInfo).first)) {
        if (bMod->getChoiceType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

ChoiceType* QatModule::getChoiceType(const String& name, const AccessInfo& reqInfo) const {
  for (auto* choiceTy : choiceTypes) {
    if ((choiceTy->getName().value == name) && choiceTy->getVisibility().isAccessible(reqInfo)) {
      return choiceTy;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasChoiceType(name, reqInfo) || sub->hasBroughtChoiceType(name, reqInfo) ||
          sub->hasAccessibleChoiceTypeInImports(name, reqInfo).first) {
        return sub->getChoiceType(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtChoiceTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasChoiceType(name, reqInfo) || bMod->hasBroughtChoiceType(name, reqInfo) ||
            bMod->hasAccessibleChoiceTypeInImports(name, reqInfo).first) {
          if (bMod->getChoiceType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getChoiceType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC CORE TYPE

bool QatModule::hasGenericCoreType(const String& name, AccessInfo reqInfo) const {
  for (auto* tempCTy : genericCoreTypes) {
    SHOW("Generic core type: " << tempCTy->getName().value)
    if ((tempCTy->getName().value == name) && tempCTy->getVisibility().isAccessible(reqInfo)) {
      SHOW("Found generic core type")
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericCoreType(name, reqInfo) || sub->hasBroughtGenericCoreType(name, reqInfo) ||
          sub->hasAccessibleGenericCoreTypeInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  SHOW("No generic core types named " + name + " found")
  return false;
}

bool QatModule::hasBroughtGenericCoreType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGenericCoreTypeInImports(const String&     name,
                                                                    const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericCoreType(name, reqInfo) || bMod->hasBroughtGenericCoreType(name, reqInfo) ||
            bMod->hasAccessibleGenericCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getGenericCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericCoreType* QatModule::getGenericCoreType(const String& name, const AccessInfo& reqInfo) {
  for (auto* tempCore : genericCoreTypes) {
    if ((tempCore->getName().value == name) && tempCore->getVisibility().isAccessible(reqInfo)) {
      return tempCore;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericCoreType(name, reqInfo)) {
        return sub->getGenericCoreType(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericCoreType(name, reqInfo) || bMod->hasBroughtGenericCoreType(name, reqInfo) ||
            bMod->hasAccessibleGenericCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getGenericCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getGenericCoreType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// TYPEDEF

bool QatModule::hasTypeDef(const String& name, AccessInfo reqInfo) const {
  for (auto* typ : typeDefs) {
    if ((typ->getName().value == name) && typ->isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasTypeDef(name, reqInfo) || sub->hasBroughtTypeDef(name, reqInfo) ||
          sub->hasAccessibleTypeDefInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtTypeDefs) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTypeDefInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasTypeDef(name, reqInfo) || bMod->hasBroughtTypeDef(name, reqInfo) ||
                                        bMod->hasAccessibleTypeDefInImports(name, reqInfo).first)) {
        if (bMod->getTypeDef(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

DefinitionType* QatModule::getTypeDef(const String& name, const AccessInfo& reqInfo) const {
  for (auto* tDef : typeDefs) {
    if ((tDef->getName().value == name) && tDef->isAccessible(reqInfo)) {
      return tDef;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasTypeDef(name, reqInfo) || sub->hasBroughtTypeDef(name, reqInfo) ||
          sub->hasAccessibleTypeDefInImports(name, reqInfo).first) {
        return sub->getTypeDef(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtTypeDefs) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTypeDef(name, reqInfo) || bMod->hasBroughtTypeDef(name, reqInfo) ||
            bMod->hasAccessibleTypeDefInImports(name, reqInfo).first) {
          if (bMod->getTypeDef(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getTypeDef(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC TYPEDEF

bool QatModule::hasGenericTypeDef(const String& name, AccessInfo reqInfo) const {
  for (auto* tempCTy : genericTypeDefinitions) {
    if ((tempCTy->getName().value == name) && tempCTy->getVisibility().isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericTypeDef(name, reqInfo) || sub->hasBroughtGenericTypeDef(name, reqInfo) ||
          sub->hasAccessibleGenericTypeDefInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtGenericTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGenericTypeDefInImports(const String&     name,
                                                                   const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericTypeDef(name, reqInfo) || bMod->hasBroughtGenericTypeDef(name, reqInfo) ||
            bMod->hasAccessibleGenericTypeDefInImports(name, reqInfo).first) {
          if (bMod->getGenericTypeDef(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericDefinitionType* QatModule::getGenericTypeDef(const String& name, const AccessInfo& reqInfo) {
  for (auto* tempDef : genericTypeDefinitions) {
    if ((tempDef->getName().value == name) && tempDef->getVisibility().isAccessible(reqInfo)) {
      return tempDef;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGenericTypeDef(name, reqInfo) || sub->hasBroughtGenericTypeDef(name, reqInfo) ||
          sub->hasAccessibleGenericTypeDefInImports(name, reqInfo).first) {
        return sub->getGenericTypeDef(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericTypeDef(name, reqInfo) || bMod->hasBroughtGenericTypeDef(name, reqInfo) ||
            bMod->hasAccessibleGenericTypeDefInImports(name, reqInfo).first) {
          if (bMod->getGenericTypeDef(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getGenericTypeDef(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GLOBAL ENTITY

bool QatModule::hasGlobalEntity(const String& name, AccessInfo reqInfo) const {
  for (auto* entity : globalEntities) {
    if ((entity->getName().value == name) && entity->getVisibility().isAccessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGlobalEntity(name, reqInfo) || sub->hasBroughtGlobalEntity(name, reqInfo) ||
          sub->hasAccessibleGlobalEntityInImports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool QatModule::hasBroughtGlobalEntity(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGlobalEntities) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGlobalEntityInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasGlobalEntity(name, reqInfo) || bMod->hasBroughtGlobalEntity(name, reqInfo) ||
           bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

GlobalEntity* QatModule::getGlobalEntity(const String&     name, // NOLINT(misc-no-recursion)
                                         const AccessInfo& reqInfo) const {
  for (auto* ent : globalEntities) {
    if ((ent->getName().value == name) && ent->getVisibility().isAccessible(reqInfo)) {
      return ent;
    }
  }
  for (auto sub : submodules) {
    if (!sub->shouldPrefixName()) {
      if (sub->hasGlobalEntity(name, reqInfo) || sub->hasBroughtGlobalEntity(name, reqInfo) ||
          sub->hasAccessibleGlobalEntityInImports(name, reqInfo).first) {
        return sub->getGlobalEntity(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGlobalEntities) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasGlobalEntity(name, reqInfo) || bMod->hasBroughtGlobalEntity(name, reqInfo) ||
           bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return bMod->getGlobalEntity(name, reqInfo);
        }
      }
    }
  }
  return nullptr;
}

bool QatModule::isParentModuleOf(QatModule* other) const {
  auto* othMod = other->parent;
  if (othMod) {
    while (othMod) {
      if (getID() == othMod->getID()) {
        return true;
      } else {
        othMod = othMod->parent;
      }
    }
    return false;
  } else {
    return false;
  }
}

bool QatModule::hasClosestParentBox() const {
  if (moduleType == ModuleType::box) {
    return true;
  } else {
    if (parent) {
      return parent->hasClosestParentBox();
    } else {
      return false;
    }
  }
}

QatModule* QatModule::getClosestParentBox() {
  if (moduleType == ModuleType::box) {
    return this;
  } else {
    if (parent) {
      return parent->getClosestParentBox();
    } else {
      return nullptr;
    }
  }
}

bool QatModule::hasClosestParentLib() const {
  if (moduleType == ModuleType::lib) {
    return true;
  } else {
    if (parent) {
      return parent->hasClosestParentLib();
    } else {
      return false;
    }
  }
}

QatModule* QatModule::getClosestParentLib() {
  if (moduleType == ModuleType::lib) {
    return this;
  } else {
    if (parent) {
      return parent->getClosestParentLib();
    } else {
      return nullptr;
    }
  }
}

String QatModule::getFilePath() const { return filePath.string(); }

bool QatModule::areNodesEmitted() const { return isEmitted; }

void QatModule::createModules(IR::Context* ctx) {
  if (!hasCreatedModules) {
    hasCreatedModules = true;
    SHOW("Creating modules via nodes \nname = " << name.value << "\n    Path = " << filePath.string())
    SHOW("    hasParent = " << (parent != nullptr))
    auto* oldMod = ctx->setActiveModule(this);
    SHOW("Submodule count before creating modules via nodes: " << submodules.size())
    for (auto* node : nodes) {
      node->createModule(ctx);
    }
    SHOW("Submodule count after creating modules via nodes: " << submodules.size())
    for (auto* sub : submodules) {
      sub->createModules(ctx);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::handleFilesystemBrings(IR::Context* ctx) {
  if (!hasHandledFilesystemBrings) {
    hasHandledFilesystemBrings = true;
    auto* oldMod               = ctx->setActiveModule(this);
    for (auto* node : nodes) {
      node->handleFilesystemBrings(ctx);
    }
    for (auto* sub : submodules) {
      sub->handleFilesystemBrings(ctx);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::handleBrings(IR::Context* ctx) {
  if (!hasHandledBrings) {
    hasHandledBrings = true;
    auto* oldMod     = ctx->setActiveModule(this);
    for (auto* node : nodes) {
      node->handleBrings(ctx);
    }
    for (auto* sub : submodules) {
      sub->handleBrings(ctx);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::defineTypes(IR::Context* ctx) {
  if (!hasDefinedTypes) {
    hasDefinedTypes = true;
    SHOW("Defining types")
    auto* oldMod = ctx->setActiveModule(this);
    for (auto& node : nodes) {
      node->defineType(ctx);
      if (((node->nodeType() == ast::NodeType::defineCoreType) && ((ast::DefineCoreType*)node)->isGeneric()) ||
          ((node->nodeType() == ast::NodeType::typeDefinition) && ((ast::TypeDefinition*)node)->isGeneric())) {
        node = new ast::HolderNode(node);
      }
    }
    for (auto* sub : submodules) {
      sub->defineTypes(ctx);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::defineNodes(IR::Context* ctx) {
  if (!hasDefinedNodes) {
    hasDefinedNodes = true;
    SHOW("Defining nodes")
    auto* oldMod = ctx->setActiveModule(this);
    for (auto& node : nodes) {
      node->define(ctx);
      if ((node->nodeType() == ast::NodeType::functionDefinition) && (((ast::FunctionDefinition*)node)->isGeneric())) {
        node = new ast::HolderNode(node);
      } else if ((node->nodeType() == ast::NodeType::functionPrototype) &&
                 (((ast::FunctionPrototype*)node)->isGeneric())) {
        node = new ast::HolderNode(node);
      }
    }
    for (auto* sub : submodules) {
      sub->defineNodes(ctx);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::emitNodes(IR::Context* ctx) {
  if (!isEmitted) {
    isEmitted    = true;
    auto* oldMod = ctx->setActiveModule(this);
    SHOW("About to emit for module: " << getFullName())
    for (auto* node : nodes) {
      if (node) {
        SHOW("Emitting node with type: " << (int)node->nodeType())
        (void)node->emit(ctx);
      }
    }
    SHOW("Emission for module complete")
    for (auto* sub : submodules) {
      SHOW("About to emit for submodule: " << sub->getFullName())
      sub->emitNodes(ctx);
    }
    SHOW("Emitted submodules")
    if (moduleInitialiser) {
      moduleInitialiser->getBlock()->setActive(ctx->builder);
      ctx->builder.CreateRetVoid();
    }
    SHOW("Module type is: " << (int)moduleType)
    auto* cfg = cli::Config::get();
    SHOW("Creating llvm output path")
    auto fileName = getWritableName() + ".ll";
    if (!cfg->hasOutputPath()) {
      fs::remove_all(basePath / "llvm");
    }
    llPath = (cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) / "llvm" /
             filePath.lexically_relative(basePath).replace_filename(fileName);
    std::error_code errorCode;
    SHOW("Creating all folders in llvm output path: " << llPath)
    fs::create_directories(llPath.parent_path(), errorCode);
    if (!errorCode) {
      errorCode.clear();
      llvm::raw_fd_ostream fStream(llPath.string(), errorCode);
      if (!errorCode) {
        SHOW("Printing LLVM module")
        llvmModule->print(fStream, nullptr);
        SHOW("LLVM module printed")
        ctx->llvmOutputPaths.push_back(llPath);
        SHOW("Number of llvm output paths is: " << ctx->llvmOutputPaths.size())
      } else {
        SHOW("Could not open file for writing")
      }
      fStream.flush();
      SHOW("Flushed llvm IR file stream")
    } else {
      ctx->Error("Error while creating parent directory for LLVM output file with path: " +
                     ctx->highlightError(llPath.parent_path().string()) + " with error: " + errorCode.message(),
                 None);
    }
    (void)ctx->setActiveModule(oldMod);
  }
}

void QatModule::compileToObject(IR::Context* ctx) {
  if (!isCompiledToObject) {
    SHOW("Compiling Module `" << name.value << "` from file " << filePath.string())
    auto*  cfg = cli::Config::get();
    String compileCommand(cfg->hasClangPath() ? (fs::absolute(cfg->getClangPath()).string() + " ") : "clang ");
    if (cfg->shouldBuildShared()) {
      compileCommand.append("-fPIC -c ");
    } else {
      compileCommand.append("-c ");
    }
    if (moduleInfo.linkPthread) {
      compileCommand += "-pthread ";
    }
    compileCommand.append("--target=").append(ctx->clangTargetInfo->getTriple().getTriple()).append(" ");
    if (cfg->hasSysroot()) {
      compileCommand.append("--sysroot=").append(cfg->getSysroot()).append(" ");
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      compileCommand.append("-nostartfiles -Wl,--no-entry -Wl,--export-all ");
    }
    for (auto* sub : submodules) {
      sub->compileToObject(ctx);
    }
    // FIXME - Also link modules of other brought entities
    objectFilePath = fs::absolute((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) / "object" /
                                  filePath.lexically_relative(basePath).replace_filename(getWritableName().append(
                                      ctx->clangTargetInfo->getTriple().isOSWindows()
                                          ? ".obj"
                                          : (ctx->clangTargetInfo->getTriple().isWasm() ? ".wasm" : ".o"))))
                         .lexically_normal();
    auto currTime   = std::chrono::system_clock::now();
    auto currTime_t = std::chrono::system_clock::to_time_t(currTime);

    struct std::tm*   currTime_ctime = std::localtime(&currTime_t);
    std::stringstream dateTimeBuff;
    dateTimeBuff << std::put_time(currTime_ctime, "%Y%m%d%H%M%S");
    std::error_code errorCode;
    SHOW("Creating all folders in object file output path: " << objectFilePath.value())
    fs::create_directories(objectFilePath.value().parent_path(), errorCode);
    if (!errorCode) {
      compileCommand.append(llPath.string()).append(" -o ").append(objectFilePath.value().string());
      SHOW("Command is: " << compileCommand)
      if (std::system(compileCommand.c_str())) {
        ctx->Error("Could not compile the LLVM file: " + ctx->highlightError(filePath.string()), None);
      }
      isCompiledToObject = true;
    } else {
      ctx->Error("Could not create parent directory for the object file. Parent directory path is: " +
                     ctx->highlightError(objectFilePath.value().parent_path().string()) +
                     " with error: " + errorCode.message(),
                 None);
    }
  }
}

std::set<String> QatModule::getAllObjectPaths() const {
  std::set<String> result;
  if (objectFilePath.has_value()) {
    result.insert(objectFilePath.value());
  }
  auto moduleHandler = [&](IR::QatModule* modVal) {
    auto objList = modVal->getAllObjectPaths();
    for (auto& path : objList) {
      result.insert(path);
    }
  };
  for (auto sub : submodules) {
    moduleHandler(sub);
  }
  for (auto& bMod : broughtModules) {
    moduleHandler(bMod.get());
  }
  for (auto& bTy : broughtCoreTypes) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtGenericCoreTypes) {
    moduleHandler(bTy.get()->getModule());
  }
  for (auto& bTy : broughtChoiceTypes) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtOpaqueTypes) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtGenericOpaqueTypes) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtMixTypes) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtTypeDefs) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtFunctions) {
    moduleHandler(bTy.get()->getParentModule());
  }
  for (auto& bTy : broughtGenericFunctions) {
    moduleHandler(bTy.get()->getModule());
  }
  for (auto& bTy : broughtGenericTypeDefinitions) {
    moduleHandler(bTy.get()->getModule());
  }
  for (auto& bTy : broughtGlobalEntities) {
    moduleHandler(bTy.get()->getParent());
  }
  for (auto& bTy : broughtRegions) {
    moduleHandler(bTy.get()->getParent());
  }
  return result;
}

void QatModule::bundleLibs(IR::Context* ctx) {
  if (!isBundled) {
    for (auto* sub : submodules) {
      sub->bundleLibs(ctx);
    }
    auto*       cfg = cli::Config::get();
    String      cmdOne;
    String      targetCMD;
    Vec<String> libsToLink;
    Vec<String> staticLibsToLink;
    Vec<String> sharedLibsToLink;
    if (moduleInfo.linkPthread) {
      cmdOne.append("-pthread ");
    }
    auto hostTriplet = llvm::Triple(LLVM_HOST_TRIPLE);
    if (!moduleInfo.nativeLibsToLink.empty()) {
      for (auto& nLib : moduleInfo.nativeLibsToLink) {
        if (nLib.isName() || nLib.isLibPath()) {
          bool foundLib        = false;
          auto tripletToString = [&]() {
            Vec<String> result;
            auto        triplet = cfg->hasTargetTriple() ? ctx->clangTargetInfo->getTriple() : hostTriplet;
            result.push_back(
                (triplet.getArchName() + "-" + triplet.getOSName() + "-" + triplet.getEnvironmentName()).str());
            const bool isArchKnown = triplet.getArch() != llvm::Triple::ArchType::UnknownArch;
            const bool isOSKnown   = triplet.getOS() != llvm::Triple::OSType::UnknownOS;
            const bool isEnvKnown  = triplet.getEnvironment() != llvm::Triple::EnvironmentType::UnknownEnvironment;
            result.push_back(((isArchKnown ? triplet.getArchName() + "-" : "") +
                              (isOSKnown ? triplet.getOSName() + (isEnvKnown ? "-" : "") : "") +
                              (isEnvKnown ? triplet.getEnvironmentName() : ""))
                                 .str());
            result.push_back(triplet.getTriple());
            return result;
          };
          if (cfg->hasTargetTriple() ? !ctx->clangTargetInfo->getTriple().isOSWindows() : !hostTriplet.isOSWindows()) {
            if (nLib.isName()) {
              Vec<String> searchPaths = {"/usr/local/lib", "/usr/local/lib32", "/usr/local/lib64",
                                         "/usr/lib",       "/usr/lib32",       "/usr/lib64",
                                         "/lib",           "/lib32",           "/lib64"};
              for (auto const& triplet : tripletToString()) {
                searchPaths.push_back("/usr/lib/" + triplet);
                searchPaths.push_back("/usr/lib32/" + triplet);
                searchPaths.push_back("/usr/lib64/" + triplet);
                searchPaths.push_back("/lib/" + triplet);
                searchPaths.push_back("/lib32/" + triplet);
                searchPaths.push_back("/lib64/" + triplet);
              }
              for (auto& sPath : searchPaths) {
                if (fs::exists(sPath)) {
                  auto libPathStatic1 =
                      (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                      ("lib" + nLib.name.value().value +
                       ((nLib.name.value().value.find_last_of(".a") == nLib.name.value().value.length() - 2) ? ""
                                                                                                             : ".a"));
                  auto libPathStatic2 =
                      (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                      (nLib.name.value().value +
                       (nLib.name.value().value.find_last_of(".a") == (nLib.name.value().value.length() - 2) ? ""
                                                                                                             : ".a"));
                  auto libPathShared1 =
                      (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                      ("lib" + nLib.name.value().value +
                       ((nLib.name.value().value.find_last_of(".so") == nLib.name.value().value.length() - 3) ? ""
                                                                                                              : ".so"));
                  auto libPathShared2 =
                      (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                      (nLib.name.value().value +
                       (nLib.name.value().value.find_last_of(".so") == (nLib.name.value().value.length() - 3) ? ""
                                                                                                              : ".so"));
                  auto libPathShared3 =
                      (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                      ("lib" + nLib.name.value().value +
                       ((nLib.name.value().value.find_last_of(".dylib") == nLib.name.value().value.length() - 6)
                            ? ""
                            : ".dylib"));
                  auto libPathShared4 = (cfg->hasSysroot() ? (fs::path(cfg->getSysroot()) / sPath) : fs::path(sPath)) /
                                        (nLib.name.value().value + (nLib.name.value().value.find_last_of(".dylib") ==
                                                                            (nLib.name.value().value.length() - 6)
                                                                        ? ""
                                                                        : ".dylib"));
                  if (cfg->shouldBuildStatic()) {
                    if (fs::exists(libPathStatic1)) {
                      libsToLink.push_back(libPathStatic1.string());
                      foundLib = true;
                      break;
                    } else if (fs::exists(libPathStatic2)) {
                      libsToLink.push_back(libPathStatic2.string());
                      foundLib = true;
                      break;
                    } else {
                      if (cfg->hasSysroot() && fs::is_directory(cfg->getSysroot())) {
                        for (auto const& child : fs::recursive_directory_iterator(cfg->getSysroot())) {
                          if (child.is_regular_file()) {
                            if (child.path().has_filename()) {
                              auto fileName = child.path().filename().string();
                              auto libName  = nLib.name.value().value;
                              if ((fileName ==
                                   "lib" + libName + ((libName.find(".a") == (libName.length() - 2)) ? "" : ".a")) ||
                                  (fileName ==
                                   (libName + ((libName.find(".a") == (libName.length() - 2)) ? "" : ".a")))) {
                                libsToLink.push_back(child.path().string());
                                foundLib = true;
                                break;
                              }
                            }
                          }
                        }
                      }
                      if (!foundLib) {
                        ctx->Warning(
                            "Could not find static library named " + ctx->highlightWarning(nLib.name->value) +
                                " in the default search paths. Please make sure that the static library is installed. Shared version of the library will be used instead if present",
                            nLib.name.value().range);
                      }
                    }
                  }
                  if (cfg->shouldBuildShared() || !foundLib) {
                    if (fs::exists(libPathShared1)) {
                      if (foundLib) {
                        staticLibsToLink.push_back(libsToLink.back());
                        libsToLink.pop_back();
                        sharedLibsToLink.push_back(libPathShared1.string());
                      } else {
                        libsToLink.push_back(libPathShared1.string());
                        foundLib = true;
                      }
                      break;
                    } else if (fs::exists(libPathShared2)) {
                      if (foundLib) {
                        staticLibsToLink.push_back(libsToLink.back());
                        libsToLink.pop_back();
                        sharedLibsToLink.push_back(libPathShared2.string());
                      } else {
                        libsToLink.push_back(libPathShared2.string());
                        foundLib = true;
                      }
                      break;
                    } else if (hostTriplet.isMacOSX() && fs::exists(libPathShared3)) {
                      if (foundLib) {
                        staticLibsToLink.push_back(libsToLink.back());
                        libsToLink.pop_back();
                        sharedLibsToLink.push_back(libPathShared3.string());
                      } else {
                        libsToLink.push_back(libPathShared3.string());
                        foundLib = true;
                      }
                      break;
                    } else if (hostTriplet.isMacOSX() && fs::exists(libPathShared4)) {
                      if (foundLib) {
                        staticLibsToLink.push_back(libsToLink.back());
                        libsToLink.pop_back();
                        sharedLibsToLink.push_back(libPathShared4.string());
                      } else {
                        libsToLink.push_back(libPathShared4.string());
                        foundLib = true;
                      }
                      break;
                    } else {
                      ctx->Warning(
                          "Could not find shared library named " + ctx->highlightWarning(nLib.name->value) +
                              " in the default search paths. Please make sure that the shared library is installed",
                          nLib.name->range);
                    }
                  }
                }
              }
              if (!foundLib) {
                if (cfg->shouldBuildStatic()) {
                  ctx->Error(
                      "Could not find the static library named " + ctx->highlightError(nLib.name->value) +
                          " in the default search paths. Tried to find the shared library with the same name, but that file could also not be found."
                          " Please make sure that the static library is installed, or provide path to the library file",
                      nLib.name->range);
                } else {
                  ctx->Error(
                      "Could not find the shared library named " + ctx->highlightError(nLib.name->value) +
                          " in the default search paths. Please make sure that the shared library is installed, or provide path to the library file",
                      nLib.name->range);
                }
              }
            } else {
              auto libName = nLib.name.value();
              auto libPath = nLib.path.value();
              for (const auto& child : fs::recursive_directory_iterator(libPath.first)) {
                if (child.is_regular_file()) {
                  if (child.path().has_filename()) {
                    auto fileName = child.path().filename().string();
                    if (cfg->shouldBuildStatic()) {
                      if ((fileName == "lib" + libName.value +
                                           ((libName.value.find(".a") == (libName.value.length() - 2)) ? "" : ".a")) ||
                          (fileName == (libName.value +
                                        ((libName.value.find(".a") == (libName.value.length() - 2)) ? "" : ".a")))) {
                        libsToLink.push_back(child.path().string());
                        foundLib = true;
                        break;
                      }
                    }
                    if (cfg->shouldBuildShared() || !foundLib) {
                      if ((fileName ==
                           "lib" + libName.value +
                               ((libName.value.find(".so") == (libName.value.length() - 3)) ? "" : ".so")) ||
                          (fileName == (libName.value +
                                        ((libName.value.find(".so") == (libName.value.length() - 3)) ? "" : ".so")))) {
                        if (foundLib) {
                          staticLibsToLink.push_back(libsToLink.back());
                          libsToLink.pop_back();
                          sharedLibsToLink.push_back(child.path().string());
                        } else {
                          libsToLink.push_back(child.path().string());
                          foundLib = true;
                        }
                        break;
                      } else if (hostTriplet.isMacOSX() &&
                                 ((fileName == "lib" + libName.value +
                                                   ((libName.value.find(".dylib") == (libName.value.length() - 6))
                                                        ? ""
                                                        : ".dylib")) ||
                                  (fileName ==
                                   (libName.value + ((libName.value.find(".dylib") == (libName.value.length() - 6))
                                                         ? ""
                                                         : ".dylib"))))) {
                        if (foundLib) {
                          staticLibsToLink.push_back(libsToLink.back());
                          libsToLink.pop_back();
                          sharedLibsToLink.push_back(child.path().string());
                        } else {
                          libsToLink.push_back(child.path().string());
                          foundLib = true;
                        }
                      }
                    }
                  }
                }
              }
              if (!foundLib) {
                if (cfg->shouldBuildStatic()) {
                  ctx->Error(
                      "Could not find the static library named " + ctx->highlightError(nLib.name->value) +
                          " in the provided path " + ctx->highlightError(nLib.path->first) +
                          ". Tried to find the shared library with the same name, but that file could also not be found."
                          " Please make sure that the shared library is installed, or provide path to the library file",
                      nLib.fileRange);
                } else {
                  ctx->Error(
                      "Could not find the shared library named " + ctx->highlightError(nLib.name->value) +
                          " in the provided path " + ctx->highlightError(nLib.path->first) +
                          ". Please make sure that the shared library is installed, or provide path to the library file",
                      nLib.fileRange);
                }
              }
            }
          }
        } else if (nLib.isLibPath()) {
          if (fs::path(nLib.path->first).is_absolute() ? fs::exists(nLib.path->first)
                                                       : fs::exists(nLib.fileRange.file / nLib.path->first)) {
            if (!fs::is_regular_file(nLib.path->first)) {
              ctx->Error("The path provided here is not a file, and hence cannot be linked as a library",
                         nLib.path->second);
            }
            libsToLink.push_back(nLib.path->first);
          } else {
            ctx->Error("The library path provided here does not exist", nLib.path->second);
          }
        } else if (nLib.isStaticAndSharedPaths()) {
          if (fs::path(nLib.path->first).is_absolute() ? fs::exists(nLib.path->first)
                                                       : fs::exists(nLib.fileRange.file / nLib.path->first)) {
            if (!fs::is_regular_file(nLib.path->first)) {
              ctx->Error("The path provided here is not a file, and hence cannot be linked as a static library",
                         nLib.path->second);
            }
            staticLibsToLink.push_back(nLib.path->first);
          } else {
            ctx->Error("The static library path provided here does not exist", nLib.path->second);
          }
          if (fs::path(nLib.sharedPath->first).is_absolute()
                  ? fs::exists(nLib.sharedPath->first)
                  : fs::exists(nLib.fileRange.file / nLib.sharedPath->first)) {
            if (!fs::is_regular_file(nLib.sharedPath->first)) {
              ctx->Error("The path provided here is not a file, and hence cannot be linked as a shared library",
                         nLib.sharedPath->second);
            }
            sharedLibsToLink.push_back(nLib.sharedPath->first);
          } else {
            ctx->Error("The shared library path provided here does not exist", nLib.path->second);
          }
        }
      }
      for (auto const& libToLink : libsToLink) {
        cmdOne.append("-l").append(libToLink).append(" ");
      }
    }
    targetCMD.append("--target=").append(cfg->getTargetTriple()).append(" ");

#define MACOS_DEFAULT_SDK_PATH "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"

    if (cfg->hasSysroot()) {
      targetCMD.append("--sysroot=").append(cfg->getSysroot()).append(" ");
    } else if (ctx->clangTargetInfo->getTriple().isTargetMachineMac()) {
      if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
        targetCMD.append("--sysroot=").append("\"" MACOS_DEFAULT_SDK_PATH "\" ");
      } else {
        ctx->Error(
            "The host triplet of the compiler is " + ctx->highlightError(LLVM_HOST_TRIPLE) +
                (hostTriplet.isMacOSX() ? "" : " which is not MacOS") + ", but the target triplet is " +
                ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                " which is identified to be for a MacOS platform. Please provide the --sysroot parameter." +
                (hostTriplet.isTargetMachineMac()
                     ? (" The default value that could have been used for the sysroot is " +
                        ctx->highlightError(MACOS_DEFAULT_SDK_PATH) +
                        " which does not exist. Please make sure that you have Command Line Tools installed by running: " +
                        ctx->highlightError("sudo xcode-select --install"))
                     : ""),
            None);
      }
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      targetCMD.append("-nostartfiles -Wl,--no-entry -Wl,--export-all ");
    }
    std::set<String> objectFiles = getAllObjectPaths();
    String           inputFiles  = "";
    for (auto& objPath : objectFiles) {
      inputFiles.append(objPath);
      inputFiles.append(" ");
    }
    SHOW("Added ll paths of all brought modules")
    if (hasMain) {
      auto outPath = ((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                      filePath.lexically_relative(basePath)
                          .replace_filename(moduleInfo.outputName.value_or(name.value))
                          .replace_extension(ctx->clangTargetInfo->getTriple().isOSWindows()
                                                 ? "exe"
                                                 : (ctx->clangTargetInfo->getTriple().isWasm() ? "wasm" : "")))
                         .string();
      ctx->executablePaths.push_back(fs::absolute(outPath).lexically_normal());
      String staticLibStr;
      String sharedLibStr;
      for (auto const& statLib : staticLibsToLink) {
        staticLibStr.append("-l").append(statLib).append(" ");
      }
      for (auto const& sharedLib : sharedLibsToLink) {
        sharedLibStr.append("-l").append(sharedLib).append(" ");
      }
      auto staticCommand = String(cfg->hasClangPath() ? cfg->getClangPath() : "clang")
                               .append(" -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
                               .append(staticLibStr)
                               .append(targetCMD)
                               .append(inputFiles);
      auto sharedCommand = String(cfg->hasClangPath() ? cfg->getClangPath() : "clang")
                               .append(" -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
                               .append(sharedLibStr)
                               .append(targetCMD)
                               .append(inputFiles);
      if (cfg->shouldBuildStatic()) {
        SHOW("Static Build Command :: " << staticCommand)
        if (std::system(staticCommand.c_str())) {
          ctx->Error("Statically linking & compiling executable failed: " + filePath.string(), None);
        }
      } else if (cfg->shouldBuildShared()) {
        SHOW("Dynamic Build Command :: " << sharedCommand)
        if (std::system(sharedCommand.c_str())) {
          ctx->Error("Dynamically linking & compiling executable failed: " + filePath.string(), None);
        }
      }
      std::ifstream file(filePath, std::ios::binary);
      auto          fsize = file.tellg();
      file.seekg(0, std::ios::end);
      fsize = file.tellg() - fsize;
      ctx->binarySizes.push_back(fsize);
      file.close();
    } else {
      auto windowsTripleToMachine = [](llvm::Triple const& triple) -> String {
        if (triple.isWindowsArm64EC()) {
          return "ARM64EC";
        } else if (triple.isAArch64()) {
          return "ARM64";
        } else if (triple.isARM()) {
          return "ARM";
        } else if (triple.getArch() == llvm::Triple::x86) {
          return "X86";
        } else if (triple.getArch() == llvm::Triple::x86_64) {
          return "X64";
        } else {
          return "";
        }
      };
      auto getExtension = [&]() {
        auto triplet = ctx->clangTargetInfo->getTriple();
        if (triplet.isOSWindows()) {
          return ".lib";
        } else if (triplet.isWasm()) {
          return ".wasm";
        } else {
          return ".a";
        }
      };
      auto getExtensionShared = [&]() {
        auto triplet = ctx->clangTargetInfo->getTriple();
        if (triplet.isOSWindows()) {
          return ".dll";
        } else if (triplet.isWasm()) {
          return ".wasm";
        } else if (triplet.isMacOSX()) {
          return ".dylib";
        } else {
          return ".so";
        }
      };
      auto outPath = fs::absolute((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                                  filePath.lexically_relative(basePath).replace_filename(
                                      moduleInfo.outputName.value_or(getWritableName()).append(getExtension())))
                         .string();
      auto outPathShared =
          fs::absolute((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                       filePath.lexically_relative(basePath).replace_filename(
                           moduleInfo.outputName.value_or(getWritableName()).append(getExtensionShared())))
              .string();
      String                   stdOutStr;
      String                   stdErrStr;
      llvm::raw_string_ostream linkerStdOut(stdOutStr);
      llvm::raw_string_ostream linkerStdErr(stdErrStr);
      if (ctx->clangTargetInfo->getTriple().isWindowsGNUEnvironment()) {
        /**
         *  Windows MinGW Linker
         */
        Vec<const char*> allArgs{"ld.lld", "-flavor", "gnu MinGW", "-r", "-o", outPath.c_str()};
        Vec<const char*> allArgsShared{"lld.lld", "-flavor", "gnu MinGW", "-r", "-o", outPathShared.c_str()};
        Vec<String>      staticCmdList;
        Vec<String>      sharedCmdList;
        for (auto& statLib : staticLibsToLink) {
          staticCmdList.push_back("-l" + statLib);
          allArgs.push_back(staticCmdList.back().c_str());
        }
        for (auto& sharedLib : sharedLibsToLink) {
          sharedCmdList.push_back("-l" + sharedLib);
          allArgsShared.push_back(sharedCmdList.back().c_str());
        }
        if (cfg->isReleaseMode()) {
          allArgs.push_back("--strip-debug");
          allArgsShared.push_back("--strip-debug");
        }
        allArgsShared.push_back("-Bdynamic");
        auto sysRootStr = String("--sysroot=\"").append(cfg->hasSysroot() ? cfg->getSysroot() : "").append("\"");
        if (cfg->hasSysroot()) {
          allArgs.push_back(sysRootStr.c_str());
          allArgsShared.push_back(sysRootStr.c_str());
        } else if (!tripleIsEquivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("Please provide the --sysroot parameter to build the library for the MinGW target triple " +
                         ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " which is not compatible with the compiler's host triplet " +
                         ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()),
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        if (cfg->shouldBuildStatic()) {
          SHOW("Linking Static Library")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::MinGW, &lld::mingw::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->highlightError(filePath.string()) +
                           " failed with return code " + std::to_string(result.retCode) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->shouldBuildShared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::MinGW, &lld::mingw::link}});
          if (resultShared.retCode != 0) {
            ctx->Error("Building shared library for module " + ctx->highlightError(filePath.string()) +
                           " failed with return code " + std::to_string(resultShared.retCode) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSWindows()) {
        /**
         *  Windows Linker
         */
        Vec<const char*> allArgs{"lld.link", "-flavor", "link", "/SUBSYSTEM:CONSOLE", ("/OUT:" + outPath).c_str()};
        Vec<const char*> allArgsShared{"lld.link", "-flavor", "link", "/SUBSYSTEM:CONSOLE",
                                       ("/OUT:" + outPathShared).c_str()};
        if (cfg->isReleaseMode()) {
          allArgs.push_back("/RELEASE");
          allArgsShared.push_back("/RELEASE");
        } else {
          allArgs.push_back("/DEBUG");
          allArgsShared.push_back("/DEBUG");
        }
        allArgsShared.push_back("/DLL");
        auto machineStr = "/MACHINE:" + windowsTripleToMachine(ctx->clangTargetInfo->getTriple());
        if (cfg->hasTargetTriple()) {
          allArgs.push_back(machineStr.c_str());
          allArgsShared.push_back(machineStr.c_str());
        }
        auto libPathStr = "/LIBPATH:\"" + (cfg->hasSysroot() ? cfg->getSysroot() : "") + "\"";
        if (cfg->hasSysroot()) {
          allArgs.push_back(libPathStr.c_str());
          allArgsShared.push_back(libPathStr.c_str());
        } else if (!tripleIsEquivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("The target triplet " + ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " is not compatible with the compiler's host triplet " +
                         ctx->highlightError(hostTriplet.getTriple()) +
                         ". Please provide the --sysroot parameter with the path to the SDK for the target platform",
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        for (auto& statLib : staticLibsToLink) {
          allArgs.push_back(statLib.c_str());
        }
        for (auto& sharedLib : sharedLibsToLink) {
          allArgsShared.push_back(sharedLib.c_str());
        }
        if (cfg->shouldBuildStatic()) {
          SHOW("Linking Static Library")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::WinLink, &lld::coff::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->shouldBuildShared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::WinLink, &lld::coff::link}});
          if (resultShared.retCode != 0) {
            ctx->Error("Building shared library for module " + ctx->highlightError(filePath.string()) +
                           " failed with return code " + std::to_string(resultShared.retCode) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatELF()) {
        /**
         *  ELF Linker
         */
        Vec<const char*> allArgs{"ld.lld", "-flavor", "gnu", "-r", "-o", outPath.c_str()};
        Vec<const char*> allArgsShared{"ld.lld", "-flavor", "gnu", "-r", "-o", outPathShared.c_str()};
        Vec<String>      staticCmdList;
        Vec<String>      sharedCmdList;
        for (auto& statLib : staticLibsToLink) {
          staticCmdList.push_back("-l" + statLib);
          allArgs.push_back(staticCmdList.back().c_str());
        }
        for (auto& sharedLib : sharedLibsToLink) {
          sharedCmdList.push_back("-l" + sharedLib);
          allArgsShared.push_back(sharedCmdList.back().c_str());
        }
        if (cfg->isReleaseMode()) {
          allArgs.push_back("--strip-debug");
          allArgsShared.push_back("--strip-debug");
        }
        allArgsShared.push_back("-Bdynamic");
        String libPathStr = String("--library-path=\"").append(cfg->hasSysroot() ? cfg->getSysroot() : "").append("\"");
        if (cfg->hasSysroot()) {
          allArgs.push_back(libPathStr.c_str());
          allArgsShared.push_back(libPathStr.c_str());
        } else if (!tripleIsEquivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("The target triple " + ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " is not compatible with the compiler's host triplet " +
                         ctx->highlightError(hostTriplet.getTriple()) +
                         ". Please provide the --sysroot parameter with the path to the SDK for the target platform",
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        if (cfg->shouldBuildStatic()) {
          SHOW("Linking Static Library")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Gnu, &lld::elf::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->shouldBuildShared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::Gnu, &lld::elf::link}});
          if (resultShared.retCode != 0) {
            ctx->Error("Building shared library for module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker's error can be found in " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatWasm()) {
        /**
         *  WASM Linker
         */
        Vec<const char*> allArgs{"wasm-ld", "-flavor", "wasm", "--export-dynamic", "-o", outPath.c_str()};
        if (cfg->hasSysroot()) {
          allArgs.push_back(String("--library-path=\"").append(cfg->getSysroot()).append("\"").c_str());
        } else if (!hostTriplet.isWasm()) {
          ctx->Error("Please provide the --sysroot parameter to build the library for the WASM target " +
                         ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()),
                     None);
        }
        Vec<String> staticCmdList;
        for (auto& statLib : staticLibsToLink) {
          staticCmdList.push_back("-l" + statLib);
          allArgs.push_back(staticCmdList.back().c_str());
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
        }
        if (cfg->shouldBuildStatic()) {
          SHOW("Linking Static Library")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Wasm, &lld::wasm::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static WASM library for module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->shouldBuildShared()) {
          ctx->Error(
              "Cannot build shared libraries for WASM. Please see link: " +
                  ctx->highlightError("https://github.com/WebAssembly/tool-conventions/blob/main/DynamicLinking.md"),
              None);
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatMachO()) {
        /**
         *  MacOS Linker
         */
        Vec<const char*> allArgs{"ld64.lld", "-flavor", "darwin", "-o", outPath.c_str()};
        Vec<const char*> allArgsShared{"ld64.lld", "-flavor", "darwin", "-o", outPathShared.c_str()};
        Vec<String>      staticCmdList;
        Vec<String>      sharedCmdList;
        for (auto& statLib : staticLibsToLink) {
          staticCmdList.push_back("-l" + statLib);
          allArgs.push_back(staticCmdList.back().c_str());
        }
        for (auto& sharedLib : sharedLibsToLink) {
          sharedCmdList.push_back("-l" + sharedLib);
          allArgsShared.push_back(sharedCmdList.back().c_str());
        }
        allArgsShared.push_back("-dynamic");
        if (cfg->hasTargetTriple()) {
          allArgs.push_back(("-arch=" + ctx->clangTargetInfo->getTriple().getArchName().str()).c_str());
          allArgsShared.push_back(("-arch=" + ctx->clangTargetInfo->getTriple().getArchName().str()).c_str());
        }
        auto sysrootStr = String("-syslibroot=\"").append(cfg->hasSysroot() ? cfg->getSysroot() : "").append("\"");
        if (cfg->hasSysroot()) {
          allArgs.push_back(sysrootStr.c_str());
          allArgsShared.push_back(sysrootStr.c_str());
        } else if (ctx->clangTargetInfo->getTriple().isMacOSX()) {
          if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
            sysrootStr = String("-syslibroot=").append("\"" MACOS_DEFAULT_SDK_PATH "\"");
            allArgs.push_back(sysrootStr.c_str());
            allArgsShared.push_back(sysrootStr.c_str());
          } else {
            ctx->Error("The compiler's host triplet is " + ctx->highlightError(LLVM_HOST_TRIPLE) +
                           (hostTriplet.isMacOSX() ? "" : " which is not MacOS") + ", but the target triplet is " +
                           ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                           " which is identified to be for a MacOS platform. Please provide the --sysroot parameter." +
                           (hostTriplet.isTargetMachineMac()
                                ? (" The default value that could have been used for the sysroot is " +
                                   ctx->highlightError(MACOS_DEFAULT_SDK_PATH) + " which does not exist")
                                : ""),
                       None);
          }
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
        }
        if (cfg->shouldBuildStatic()) {
          SHOW("Linking Static Library")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Darwin, &lld::macho::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->highlightError(filePath.string()) +
                           " failed with return code " + ctx->highlightError(std::to_string(result.retCode)) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->shouldBuildShared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::Darwin, &lld::macho::link}});
          if (resultShared.retCode != 0) {
            ctx->Error("Building shared library for module " + ctx->highlightError(filePath.string()) +
                           " failed with return code " + ctx->highlightError(std::to_string(resultShared.retCode)) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else {
        if (cfg->hasLinkerPath()) {
          auto cmd = String(cfg->getLinkerPath()).append(" -o ").append(outPath).append(" ");
          for (auto& obj : objectFiles) {
            cmd.append(obj).append(" ");
          }
          if (std::system(cmd.c_str())) {
            ctx->Error("Building library for module " + ctx->highlightError(filePath.string()) + " failed.", None);
          }
        } else {
          ctx->Error("Could not find the linker to be used for the target " +
                         ctx->highlightError(ctx->clangTargetInfo->getTriple().getTriple()) +
                         ". Please provide the --linker argument with the path to the linker to use",
                     None);
        }
      }
    }
    isBundled = true;
  }
}

bool QatModule::hasMainFn() const { return hasMain; }

void QatModule::setHasMainFn() { hasMain = true; }

fs::path QatModule::getResolvedOutputPath(const String& extension, IR::Context* ctx) {
  auto* config = cli::Config::get();
  auto  fPath  = filePath;
  auto  out    = fPath.replace_extension(extension);
  if (config->hasOutputPath()) {
    out = (config->getOutputPath() / filePath.lexically_relative(basePath).remove_filename());
    std::error_code errorCode;
    fs::create_directories(out, errorCode);
    if (!errorCode) {
      out = out / fPath.filename();
    } else {
      ctx->Error("Could not create the parent directory of the output files with path: " +
                     ctx->highlightError(out.string()) + " with error: " + errorCode.message(),
                 None);
    }
  }
  return out;
}

void QatModule::exportJsonFromAST(IR::Context* ctx) {
  if ((moduleType == ModuleType::file) || rootLib) {
    auto*          cfg    = cli::Config::get();
    auto           result = Json();
    Vec<JsonValue> contents;
    for (auto* node : nodes) {
      contents.push_back(node->toJson());
    }
    result["contents"] = contents;
    std::fstream jsonStream;
    auto         jsonPath =
        (cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) / "AST" /
        filePath.lexically_relative(basePath).replace_filename(filePath.filename().string()).replace_extension("json");
    std::error_code errorCode;
    fs::create_directories(jsonPath.parent_path(), errorCode);
    if (!errorCode) {
      jsonStream.open(jsonPath, std::ios_base::out);
      if (jsonStream.is_open()) {
        jsonStream << result;
        jsonStream.close();
      } else {
        ctx->Error("Output file could not be opened for writing the JSON representation", {getParentFile()->filePath});
      }
    } else {
      ctx->Error("Could not create parent directories for the JSON file for exporting AST",
                 {getParentFile()->filePath});
    }
  } else {
    SHOW("Module type not suitable for exporting AST")
  }
}

llvm::Module* QatModule::getLLVMModule() const { return llvmModule; }

void QatModule::linkIntrinsic(IntrinsicID intr) {
  auto& llCtx = llvmModule->getContext();
  switch (intr) {
    case IntrinsicID::vaStart: {
      llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {llvm::Type::getInt8PtrTy(llCtx)}, false),
          llvm::GlobalValue::LinkageTypes::ExternalLinkage, "llvm.va_start", llvmModule);
      break;
    }
    case IntrinsicID::vaCopy: {
      llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                     {llvm::Type::getInt8PtrTy(llCtx), llvm::Type::getInt8PtrTy(llCtx)},
                                                     false),
                             llvm::GlobalValue::LinkageTypes::ExternalLinkage, "llvm.va_copy", llvmModule);
      break;
    }
    case IntrinsicID::vaEnd: {
      llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {llvm::Type::getInt8PtrTy(llCtx)}, false),
          llvm::GlobalValue::LinkageTypes::ExternalLinkage, "llvm.va_end", llvmModule);
      break;
    }
  }
}

void QatModule::linkNative(NativeUnit nval) {
  // FIXME - Use integer widths according to the specification and not the same
  // on all platforms
  llvm::LLVMContext& llCtx = llvmModule->getContext();
  switch (nval) {
    case NativeUnit::printf: {
      if (!llvmModule->getFunction("printf")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, true),
                               llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "printf", llvmModule);
      }
      break;
    }
    case NativeUnit::malloc: {
      if (!llvmModule->getFunction("malloc")) {
        SHOW("Creating malloc function")
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(),
                                                       {llvm::Type::getInt64Ty(llCtx)}, false),
                               llvm::GlobalValue::ExternalWeakLinkage, "malloc", llvmModule);
      }
      break;
    }
    case NativeUnit::free: {
      if (!llvmModule->getFunction("free")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, false),
                               llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "free", llvmModule);
      }
      break;
    }
    case NativeUnit::realloc: {
      if (!llvmModule->getFunction("realloc")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(),
                                    {llvm::Type::getInt8Ty(llCtx)->getPointerTo(), llvm::Type::getInt64Ty(llCtx)},
                                    false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "realloc", llvmModule);
      }
      break;
    }
    case NativeUnit::pthreadCreate: {
      if (!llvmModule->getFunction("pthread_create")) {
        llvm::Type* pthreadPtrTy = llvm::Type::getInt64Ty(llCtx)->getPointerTo();
        llvm::Type* voidPtrTy    = llvm::Type::getInt8Ty(llCtx)->getPointerTo();
        auto*       pthreadFnTy  = llvm::FunctionType::get(voidPtrTy, {voidPtrTy}, false);
        if (!llvm::StructType::getTypeByName(llCtx, "pthread_attr_t")) {
          llvm::StructType::create(
              llCtx, {llvm::Type::getInt64Ty(llCtx), llvm::ArrayType::get(llvm::Type::getInt8Ty(llCtx), 48u)},
              "pthread_attr_t");
        }
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx),
                                    {pthreadPtrTy,
                                     llvm::StructType::getTypeByName(llCtx, "pthread_attr_t")->getPointerTo(),
                                     pthreadFnTy->getPointerTo(), voidPtrTy},
                                    false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_create", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadJoin: {
      if (!llvmModule->getFunction("pthread_join")) {
        llvm::Type* pthreadTy = llvm::Type::getInt64Ty(llCtx);
        llvm::Type* voidPtrTy = llvm::Type::getInt8Ty(llCtx)->getPointerTo();
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx), {pthreadTy, voidPtrTy->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_join", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadExit: {
      if (!llvmModule->getFunction("pthread_exit")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, false),
                               llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_exit", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadAttrInit: {
      if (!llvmModule->getFunction("pthread_attr_init")) {
        if (!llvm::StructType::getTypeByName(llCtx, "pthread_attr_t")) {
          llvm::StructType::create(
              llCtx, {llvm::Type::getInt64Ty(llCtx), llvm::ArrayType::get(llvm::Type::getInt8Ty(llCtx), 48u)},
              "pthread_attr_t");
        }
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx),
                                    {llvm::StructType::getTypeByName(llCtx, "pthread_attr_t")->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_attr_init", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
  }
}

Json QatModule::toJson() const {
  // FIXME - Change
  return Json()._("name", name);
}

// NOLINTBEGIN(readability-magic-numbers)

bool QatModule::hasIntegerBitwidth(u64 bits) const {
  if (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) {
    return true;
  } else {
    for (auto bitwidth : integerBitwidths) {
      if (bitwidth == bits) {
        return true;
      }
    }
    return false;
  }
}

void QatModule::addIntegerBitwidth(u64 bits) {
  if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64 && bits != 128) {
    bool exists = false;
    for (auto bitw : integerBitwidths) {
      if (bitw == bits) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      integerBitwidths.push_back(bits);
    }
  }
}

bool QatModule::hasUnsignedBitwidth(u64 bits) const {
  if (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) {
    return true;
  } else {
    for (auto bitwidth : unsignedBitwidths) {
      if (bitwidth == bits) {
        return true;
      }
    }
    return false;
  }
}

void QatModule::addUnsignedBitwidth(u64 bits) {
  if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64 && bits != 128) {
    bool exists = false;
    for (auto bitw : unsignedBitwidths) {
      if (bitw == bits) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      unsignedBitwidths.push_back(bits);
    }
  }
}

bool QatModule::hasFloatKind(FloatTypeKind kind) const {
  if (kind == FloatTypeKind::_32 || kind == FloatTypeKind::_64) {
    return true;
  } else {
    for (auto kindvalue : floatKinds) {
      if (kindvalue == kind) {
        return true;
      }
    }
    return false;
  }
}

void QatModule::addFloatKind(FloatTypeKind kind) {
  if (kind != FloatTypeKind::_32 && kind != FloatTypeKind::_64) {
    bool exists = false;
    for (auto kindv : floatKinds) {
      if (kindv == kind) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      floatKinds.push_back(kind);
    }
  }
}

void QatModule::finaliseModule() {
  // FIXME - Implement
}

// NOLINTEND(readability-magic-numbers)

} // namespace qat::IR
