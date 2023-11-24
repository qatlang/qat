#include "./qat_module.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/function.hpp"
#include "../ast/node.hpp"
#include "../ast/type_definition.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "function.hpp"
#include "global_entity.hpp"
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
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace qat::IR {

Vec<QatModule*> QatModule::allModules{};

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
  for (auto* sub : submodules) {
    delete sub;
  }
  for (auto* tFn : genericFunctions) {
    delete tFn;
  }
  for (auto* tCty : genericCoreTypes) {
    delete tCty;
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

Function* QatModule::createFunction(const Identifier& name, QatType* returnType, bool isAsync, Vec<Argument> args,
                                    bool isVariadic, const FileRange& fileRange, const VisibilityInfo& visibility,
                                    Maybe<llvm::GlobalValue::LinkageTypes> linkage, IR::Context* ctx) {
  SHOW("Creating IR function")
  auto* fun = Function::Create(this, name, {/* Generics */}, returnType, isAsync, std::move(args), isVariadic,
                               fileRange, visibility, ctx, linkage);
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
    integerBitsVal.push_back(JsonValue((unsigned long long)bit));
  }
  for (auto bit : unsignedBitwidths) {
    unsignedBitsVal.push_back(JsonValue((unsigned long long)bit));
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

Vec<utils::UnitNameInfo> QatModule::getLinkNameUnits() const {
  if (parent) {
    auto parentVal = parent->getLinkNameUnits();
    if (shouldPrefixName()) {
      parentVal.push_back(utils::UnitNameInfo(
          name.value, moduleType == ModuleType::box ? utils::UnitNameType::box : utils::UnitNameType::lib, this,
          moduleInfo.alternativeName));
    }
    return parentVal;
  } else {
    if (shouldPrefixName()) {
      return Vec<utils::UnitNameInfo>({utils::UnitNameInfo(
          name.value, moduleType == ModuleType::box ? utils::UnitNameType::box : utils::UnitNameType::lib, this,
          moduleInfo.alternativeName)});
    } else {
      return Vec<utils::UnitNameInfo>();
    }
  }
}

String QatModule::getLinkingName(Vec<utils::UnitNameInfo> const& names, Maybe<String> foreignID) const {
  auto isForeign = [&](String const& id) {
    if (foreignID.has_value()) {
      return (foreignID.value() == id);
    } else {
      return isInForeignModuleOfType(id);
    }
  };
  if (isForeign("C")) {
    return names.back().getUsableName();
  } else if (isForeign("C++")) {
    String result("");
    // FIXME - Implement C++ name mangling
    return result;
  } else {
    String result = "qat'";
    for (usize i = 0; i < names.size(); i++) {
      auto& unit = names.at(i);
      if (unit.namingType != utils::UnitNameType::genericList) {
        result += ":";
      }
      switch (unit.namingType) {
        case utils::UnitNameType::box: {
          result += "box_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::lib: {
          result += "lib_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::type: {
          result += "type_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::TypeDef: {
          result += "type_def_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::choice: {
          result += "type_choice_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::mix: {
          result += "type_mix_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::staticField: {
          result += "static_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::region: {
          result += "region_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::function: {
          result += "fn_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::staticFunction: {
          result += "staticfn_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::variationMethod: {
          result += "variation_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::method: {
          result += "method_";
          result += unit.getUsableName();
          break;
        }
        case utils::UnitNameType::genericList: {
          result += ":[";
          for (usize j = 0; j < unit.subUnits.size(); j++) {
            result += unit.subUnits.at(j).getUsableName();
            if (j != (unit.subUnits.size() - 1)) {
              result += ",";
            }
          }
          result += "]";
          break;
        }
      }
    }
    return result;
  }
}

bool QatModule::shouldPrefixName() const { return (moduleType == ModuleType::box || moduleType == ModuleType::lib); }

Function* QatModule::getGlobalInitialiser(IR::Context* ctx) {
  if (!moduleInitialiser) {
    moduleInitialiser =
        IR::Function::Create(this, Identifier("module'initialiser'" + utils::unique_id(), {filePath}), {/* Generics */},
                             IR::VoidType::get(ctx->llctx), false, {}, false, name.range, VisibilityInfo::pub(), ctx);
    auto* entry = new IR::Block(moduleInitialiser, nullptr);
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

bool QatModule::hasLib(const String& name) const {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtLib(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      if (!brought.isNamed()) {
        if (bMod->shouldPrefixName() && (bMod->getName() == name)) {
          return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
        }
      } else if (brought.getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
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
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name, reqInfo) ||
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
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      if (!brought.isNamed()) {
        if (((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName().value == name)) &&
            brought.getVisibility().isAccessible(reqInfo)) {
          return bMod;
        }
      } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name, reqInfo) ||
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
  if (!hasLib(name.value)) {
    addNamedSubmodule(name, filename, ModuleType::lib, visib_info, ctx);
  }
}

void QatModule::closeLib() { closeSubmodule(); }

bool QatModule::hasBox(const String& name) const {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtBox(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName().value == name)) {
          return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
        }
      } else if (brought.getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
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
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name, reqInfo) ||
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
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if (((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName().value == name)) &&
            brought.getVisibility().isAccessible(reqInfo)) {
          return bMod;
        }
      } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name, reqInfo) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getBox(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openBox(const Identifier& _name, Maybe<VisibilityInfo> visib_info, IR::Context* ctx) {
  SHOW("Opening box: " << _name.value)
  if (hasBox(_name.value)) {
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
    auto* bMod = brought.get();
    if (!brought.isNamed()) {
      if (bMod->shouldPrefixName() && (bMod->getName() == name)) {
        SHOW("Found brought module with name")
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      SHOW("Found named brought module")
      SHOW("Brought module " << bMod)
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
        SHOW("Found brought module with name")
        return bMod;
      } else if (!bMod->shouldPrefixName()) {
        if (bMod->hasBroughtModule(name, reqInfo) || bMod->hasAccessibleBroughtModuleInImports(name, reqInfo).first) {
          SHOW("Found brought module in unnamed module")
          if (bMod->getBroughtModule(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getBroughtModule(name, reqInfo);
          }
        }
      }
    } else if (brought.getName().value == name) {
      SHOW("Found named brought module")
      SHOW("Brought module " << bMod)
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

bool QatModule::hasFunction(const String& name) const {
  SHOW("Function to be checked: " << name)
  SHOW("This pointer: " << this)
  SHOW("Function count: " << functions.size())
  for (auto* function : functions) {
    SHOW("Function in module: " << function)
    if (function->getName().value == name) {
      SHOW("Found function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtFunction(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
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
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name, reqInfo) ||
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
    if (function->getName().value == name) {
      return function;
    }
  }
  for (const auto& brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if ((bFn->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bFn;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name, reqInfo) ||
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

bool QatModule::hasGenericFunction(const String& name) const {
  SHOW("Generic Function to be checked: " << name)
  SHOW("Generic Function count: " << functions.size())
  for (auto* function : genericFunctions) {
    if (function->getName().value == name) {
      SHOW("Found generic function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtGenericFunction(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
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
        if (bMod->hasGenericFunction(name) || bMod->hasBroughtGenericFunction(name, reqInfo) ||
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
    if (function->getName().value == name) {
      return function;
    }
  }
  for (const auto& brought : broughtGenericFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if ((bFn->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bFn;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericFunction(name) || bMod->hasBroughtGenericFunction(name, reqInfo) ||
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

bool QatModule::hasRegion(const String& name) const {
  for (auto* reg : regions) {
    if (reg->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtRegion(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtRegions) {
    if (!brought.isNamed()) {
      auto* reg = brought.get();
      if (reg->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleRegionInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasRegion(name) || bMod->hasBroughtRegion(name, reqInfo) ||
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
    if (reg->getName().value == name) {
      return reg;
    }
  }
  for (const auto& brought : broughtRegions) {
    if (!brought.isNamed()) {
      auto* reg = brought.get();
      if ((reg->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return reg;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasRegion(name) || bMod->hasBroughtRegion(name, reqInfo) ||
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

bool QatModule::hasOpaqueType(const String& name) const {
  SHOW("Opaque count: " << opaqueTypes.size())
  for (auto* typ : opaqueTypes) {
    if (typ->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtOpaqueType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtOpaqueTypes) {
    if (!brought.isNamed()) {
      auto* cType = brought.get();
      if (cType->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleOpaqueTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasOpaqueType(name) || bMod->hasBroughtOpaqueType(name, reqInfo) ||
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
    if (opaqueType->getName().value == name) {
      return opaqueType;
    }
  }
  for (const auto& brought : broughtOpaqueTypes) {
    if (!brought.isNamed()) {
      auto* opaque = brought.get();
      if (opaque->getName().value == name) {
        return opaque;
      }
    } else if (brought.getName().value == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasOpaqueType(name) || bMod->hasBroughtOpaqueType(name, reqInfo) ||
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

bool QatModule::hasCoreType(const String& name) const {
  SHOW("CoreType count: " << coreTypes.size())
  for (auto* typ : coreTypes) {
    if (typ->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtCoreType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto* cType = brought.get();
      if (cType->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleCoreTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name, reqInfo) ||
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
    if (coreType->getName().value == name) {
      return coreType;
    }
  }
  for (const auto& brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto* coreType = brought.get();
      if (coreType->getName().value == name) {
        return coreType;
      }
    } else if (brought.getName().value == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name, reqInfo) ||
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

bool QatModule::hasMixType(const String& name) const {
  SHOW("MixType count: " << mixTypes.size())
  for (auto* typ : mixTypes) {
    if (typ->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtMixType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtMixTypes) {
    if (!brought.isNamed()) {
      auto* uType = brought.get();
      if (uType->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleMixTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasMixType(name) || bMod->hasBroughtMixType(name, reqInfo) ||
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
    if (mixTy->getName().value == name) {
      return mixTy;
    }
  }
  for (const auto& brought : broughtMixTypes) {
    if (!brought.isNamed()) {
      auto* mixTy = brought.get();
      if ((mixTy->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return mixTy;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasMixType(name) || bMod->hasBroughtMixType(name, reqInfo) ||
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

bool QatModule::hasChoiceType(const String& name) const {
  SHOW("ChoiceType count: " << mixTypes.size())
  for (auto* typ : choiceTypes) {
    if (typ->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtChoiceType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtChoiceTypes) {
    if (!brought.isNamed()) {
      auto* uType = brought.get();
      if (uType->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleChoiceTypeInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasChoiceType(name) || bMod->hasBroughtChoiceType(name, reqInfo) ||
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
    if (choiceTy->getName().value == name) {
      return choiceTy;
    }
  }
  for (const auto& brought : broughtChoiceTypes) {
    if (!brought.isNamed()) {
      auto* choiceTy = brought.get();
      if ((choiceTy->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return choiceTy;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasChoiceType(name) || bMod->hasBroughtChoiceType(name, reqInfo) ||
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

bool QatModule::hasGenericCoreType(const String& name) const {
  for (auto* tempCTy : genericCoreTypes) {
    SHOW("Generic core type: " << tempCTy->getName().value)
    if (tempCTy->getName().value == name) {
      SHOW("Found generic core type")
      return true;
    }
  }
  SHOW("No generic core types named " + name + " found")
  return false;
}

bool QatModule::hasBroughtGenericCoreType(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericCoreTypes) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
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
        if (bMod->hasGenericCoreType(name) || bMod->hasBroughtGenericCoreType(name, reqInfo) ||
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
    if (tempCore->getName().value == name) {
      return tempCore;
    }
  }
  for (const auto& brought : broughtGenericCoreTypes) {
    if (!brought.isNamed()) {
      auto* bCTy = brought.get();
      if ((bCTy->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bCTy;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericCoreType(name) || bMod->hasBroughtGenericCoreType(name, reqInfo) ||
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

bool QatModule::hasTypeDef(const String& name) const {
  SHOW("TypeDef count: " << typeDefs.size())
  for (auto* typ : typeDefs) {
    if (typ->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto* tDef = brought.get();
      if (tDef->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTypeDefInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name, reqInfo) ||
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
    if (tDef->getName().value == name) {
      return tDef;
    }
  }
  for (const auto& brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto* tDef = brought.get();
      if ((tDef->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return tDef;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name, reqInfo) ||
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

bool QatModule::hasGenericTypeDef(const String& name) const {
  for (auto* tempCTy : genericTypeDefinitions) {
    SHOW("Generic type def: " << tempCTy->getName().value)
    if (tempCTy->getName().value == name) {
      SHOW("Found generic type def")
      return true;
    }
  }
  SHOW("No generic type defs named " + name + " found")
  return false;
}

bool QatModule::hasBroughtGenericTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
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
        if (bMod->hasGenericTypeDef(name) || bMod->hasBroughtGenericTypeDef(name, reqInfo) ||
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
    if (tempDef->getName().value == name) {
      return tempDef;
    }
  }
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (!brought.isNamed()) {
      auto* bDTy = brought.get();
      if ((bDTy->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bDTy;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasGenericTypeDef(name) || bMod->hasBroughtGenericTypeDef(name, reqInfo) ||
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

bool QatModule::hasGlobalEntity(const String& name) const {
  for (auto* entity : globalEntities) {
    if (entity->getName().value == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtGlobalEntity(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGlobalEntities) {
    if (!brought.isNamed()) {
      auto* bGlobal = brought.get();
      if (bGlobal->getName().value == name) {
        return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
      }
    } else if (brought.getName().value == name) {
      return reqInfo.has_value() ? brought.getVisibility().isAccessible(reqInfo.value()) : true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGlobalEntityInImports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name, reqInfo) ||
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
    if (ent->getName().value == name) {
      return ent;
    }
  }
  for (const auto& brought : broughtGlobalEntities) {
    auto* bGlobal = brought.get();
    if (!brought.isNamed()) {
      if ((bGlobal->getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
        return bGlobal;
      }
    } else if ((brought.getName().value == name) && brought.getVisibility().isAccessible(reqInfo)) {
      return bGlobal;
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name, reqInfo) ||
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
    std::error_code errorCode;
    SHOW("Creating all folders in object file output path: " << objectFilePath.value())
    fs::create_directories(objectFilePath.value().parent_path(), errorCode);
    if (!errorCode) {
      compileCommand.append(llPath.string())
          .append(" -o ")
          .append(objectFilePath.value().string())
          .append(PlatformIsWindows ? " > nul" : " > /dev/null");
      SHOW("Command is: " << compileCommand)
      if (std::system(compileCommand.c_str())) {
        ctx->writeJsonResult(false);
        ctx->Error("Could not compile the LLVM file: " + filePath.string(), None);
      }
      isCompiledToObject = true;
    } else {
      ctx->Error("Could not create parent directory for the Object file output: " +
                     ctx->highlightError(objectFilePath.value().parent_path().string()) +
                     " with error: " + errorCode.message(),
                 None);
    }
  }
}

void QatModule::bundleLibs(IR::Context* ctx) {
  if (!isBundled) {
    for (auto* sub : submodules) {
      sub->bundleLibs(ctx);
    }
    auto*  cfg = cli::Config::get();
    String cmdOne;
    String targetCMD;
    if (moduleInfo.linkPthread) {
      cmdOne.append("-pthread ");
    }
    targetCMD.append("--target=").append(cfg->getTargetTriple()).append(" ");
    if (cfg->hasSysroot()) {
      targetCMD.append("--sysroot=").append(cfg->getSysroot()).append(" ");
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      targetCMD.append("-nostartfiles -Wl,--no-entry -Wl,--export-all ");
    }
    Vec<String> objectFiles;
    if (objectFilePath.has_value()) {
      objectFiles.push_back(objectFilePath.value().string());
    }
    for (auto* sub : submodules) {
      if (objectFilePath.has_value()) {
        objectFiles.push_back(sub->objectFilePath.value().string());
      }
    }
    for (const auto& bMod : broughtModules) {
      SHOW("Brought module: " << bMod.get())
      SHOW("Brought module name: " << bMod.get()->name.value)
      SHOW("Brought module has Object: " << bMod.get()->objectFilePath.has_value())

      if (bMod.get()->objectFilePath.has_value()) {
        objectFiles.push_back(bMod.get()->objectFilePath.value().string());
      }
    }
    String inputFiles = "";
    for (usize i = 0; i < objectFiles.size(); i++) {
      inputFiles.append(objectFiles.at(i));
      if (i != (objectFiles.size() - 1)) {
        inputFiles.append(" ");
      }
    }
    inputFiles.append(" ");
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
      auto staticCommand = String(cfg->hasClangPath() ? cfg->getClangPath() : "clang")
                               .append(" -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
                               .append(targetCMD)
                               .append(inputFiles);
      auto sharedCommand = String(cfg->hasClangPath() ? cfg->getClangPath() : "clang")
                               .append(" -shared -fPIC -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
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
      if (cfg->shouldBuildStatic()) {
        auto outPath = fs::absolute((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                                    filePath.lexically_relative(basePath).replace_filename(
                                        moduleInfo.outputName.value_or(getWritableName())
                                            .append(ctx->clangTargetInfo->getTriple().isOSWindows() ? ".lib" : ".a")))
                           .string();
        String                   stdOutStr;
        String                   stdErrStr;
        llvm::raw_string_ostream linkerStdOut(stdOutStr);
        llvm::raw_string_ostream linkerStdErr(stdErrStr);
        if (ctx->clangTargetInfo->getTriple().isOSBinFormatCOFF()) {
          Vec<const char*> allArgs{"lld.link", "-flavor", "link", "/SUBSYSTEM:CONSOLE", ("/OUT:" + outPath).c_str()};
          for (auto& obj : objectFiles) {
            allArgs.push_back(obj.c_str());
          }
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::WinLink, &lld::coff::link}});
          if (result.retCode != 0) {
            ctx->Error("Static build of module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker output was: " + linkerStdErr.str(),
                       None);
          }
        } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatELF()) {
          Vec<const char*> allArgs{"ld.lld", "-flavor", "gnu", "-r", "-o", outPath.c_str()};
          for (auto& obj : objectFiles) {
            allArgs.push_back(obj.c_str());
          }
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Gnu, &lld::elf::link}});
          if (result.retCode != 0) {
            ctx->Error("Static build of module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker output was: " + linkerStdErr.str(),
                       None);
          }
        } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatWasm()) {
          Vec<const char*> allArgs{"wasm-ld", "-flavor", "wasm", "--export-dynamic", "-o", outPath.c_str()};
          for (auto& obj : objectFiles) {
            allArgs.push_back(obj.c_str());
          }
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Wasm, &lld::wasm::link}});
          if (result.retCode != 0) {
            ctx->Error("Static build of WASM module " + ctx->highlightError(filePath.string()) +
                           " failed. The linker output was: " + linkerStdErr.str(),
                       None);
          }
        }
        // FIXME - Support other targets
      }
      if (cfg->shouldBuildShared()) {
        auto outPath = ((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                        filePath.lexically_relative(basePath).replace_filename(
                            moduleInfo.outputName.value_or(getWritableName())
                                .append(ctx->clangTargetInfo->getTriple().isOSWindows() ? ".dll" : ".so")))
                           .string()
                           .append(" ");
        if (std::system(String(cfg->hasClangPath() ? (cfg->getClangPath() + " ") : "clang ")
                            .append(hasMain ? "-fuse-ld=lld" : "")
                            .append(" -shared -o ")
                            .append(outPath)
                            .append(cmdOne)
                            .append(inputFiles)
                            .c_str())) {
          ctx->Error("Dynamic build of module " + ctx->highlightError(filePath.string()) + " failed", None);
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
