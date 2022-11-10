#include "./qat_module.hpp"
#include "../ast/define_core_type.hpp"
#include "../ast/function.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "types/core_type.hpp"
#include "types/float.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace qat::IR {

Vec<QatModule*> QatModule::allModules{};

bool QatModule::hasFileModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::file) {
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
    if (mod->moduleType == ModuleType::file) {
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

QatModule::QatModule(String _name, fs::path _filepath, fs::path _basePath, ModuleType _type,
                     const utils::VisibilityInfo& _visibility, llvm::LLVMContext& ctx)
    : name(std::move(_name)), moduleType(_type), filePath(std::move(_filepath)), basePath(std::move(_basePath)),
      visibility(_visibility), parent(nullptr), active(nullptr) {
  llvmModule = new llvm::Module(getFullName(), ctx);
  llvmModule->setModuleIdentifier(getFullName());
  llvmModule->setSourceFileName(filePath.string());
  llvmModule->setCodeModel(llvm::CodeModel::Small);
  llvmModule->setTargetTriple(cli::Config::get()->getTargetTriple());
  allModules.push_back(this);
}

QatModule::~QatModule() = default;

QatModule* QatModule::Create(const String& name, const fs::path& filepath, const fs::path& basePath, ModuleType type,
                             const utils::VisibilityInfo& visib_info, llvm::LLVMContext& ctx) {
  return new QatModule(name, filepath, basePath, type, visib_info, ctx);
}

ModuleType QatModule::getModuleType() const { return moduleType; }

const utils::VisibilityInfo& QatModule::getVisibility() const { return visibility; }

QatModule* QatModule::getActive() { // NOLINT(misc-no-recursion)
  if (active) {
    return active->getActive();
  } else {
    return this;
  }
}

QatModule* QatModule::getParentFile() { // NOLINT(misc-no-recursion)
  if (moduleType == ModuleType::file) {
    return this;
  } else {
    if (parent) {
      return parent->getParentFile();
    }
  }
  return nullptr;
}

Pair<unsigned, String> QatModule::resolveNthParent(const String& name) const {
  u64 result = 0;
  if (name.find("..:") == 0) {
    u64 i;
    for (i = 0; i < name.length(); i++) {
      if (name.find("..:", i) == i) {
        result++;
        i += 2;
      } else {
        break;
      }
    }
    return {result, name.substr(i)};
  }
  return {result, name};
}

bool QatModule::hasNthParent( // NOLINT(misc-no-recursion)
    u32 n) const {
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

Function* QatModule::createFunction(const String& name, QatType* returnType, bool isReturnTypeVariable, bool isAsync,
                                    Vec<Argument> args, bool isVariadic, const utils::FileRange& fileRange,
                                    const utils::VisibilityInfo& visibility, llvm::GlobalValue::LinkageTypes linkage,
                                    llvm::LLVMContext& ctx) {
  SHOW("Creating IR function")
  auto* fun = Function::Create(this, name, returnType, isReturnTypeVariable, isAsync, std::move(args), isVariadic,
                               fileRange, visibility, ctx);
  SHOW("Created function")
  functions.push_back(fun);
  return fun;
}

QatModule* QatModule::CreateSubmodule(QatModule* parent, fs::path filepath, fs::path basePath, String sname,
                                      ModuleType type, const utils::VisibilityInfo& visibilityInfo,
                                      llvm::LLVMContext& ctx) {
  SHOW("Creating submodule: " << sname)
  auto* sub = new QatModule(std::move(sname), std::move(filepath), std::move(basePath), type, visibilityInfo, ctx);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
    SHOW("Created submodule")
  }
  return sub;
}

QatModule* QatModule::CreateFile(QatModule* parent, fs::path filepath, fs::path basePath, String fname,
                                 Vec<String> content, Vec<ast::Node*> nodes, utils::VisibilityInfo visibilityInfo,
                                 llvm::LLVMContext& ctx) {
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

String QatModule::getName() const { return name; }

String QatModule::getFullName() const {
  if (parent) {
    if (parent->shouldPrefixName()) {
      if (shouldPrefixName()) {
        return parent->getFullName() + ":" + name;
      } else {
        return parent->getFullName();
      }
    } else {
      if (shouldPrefixName()) {
        return name;
      } else {
        return "";
      }
    }
  } else {
    return shouldPrefixName() ? name : "";
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
      result += ("lib-" + name);
      break;
    }
    case ModuleType::box: {
      result += ("box-" + name);
      break;
    }
    case ModuleType::file: {
      result += ("file-" + filePath.stem().string());
      break;
    }
    case ModuleType::folder: {
      result += ("folder-" + name);
      break;
    }
  }
  SHOW("Writable final result is " << result)
  return result;
}

String QatModule::getFullNameWithChild(const String& child) const {
  if (shouldPrefixName()) {
    SHOW("Getting full name: " << getFullName())
    return getFullName() + ":" + child;
  } else {
    return child;
  }
}

bool QatModule::shouldPrefixName() const { return (moduleType == ModuleType::box || moduleType == ModuleType::lib); }

Function* QatModule::getGlobalInitialiser(IR::Context* ctx) {
  if (!globalInitialiser) {
    globalInitialiser = IR::Function::Create(
        this, "qat'global'initialiser", IR::VoidType::get(ctx->llctx), false, false, {}, false,
        utils::FileRange("", utils::FilePos{0u, 0u}, utils::FilePos{0u, 0u}), utils::VisibilityInfo::pub(), ctx->llctx);
    auto* entry = new IR::Block(globalInitialiser, nullptr);
    entry->setActive(ctx->builder);
  }
  return globalInitialiser;
}

bool QatModule::isSubmodule() const { return parent != nullptr; }

void QatModule::addSubmodule(const String& sname, const String& filename, ModuleType type,
                             const utils::VisibilityInfo& visib_info, llvm::LLVMContext& ctx) {
  SHOW("Creating submodule: " << sname)
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

bool QatModule::hasBroughtLib(const String& name) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      if (!brought.isNamed()) {
        if (bMod->shouldPrefixName() && (bMod->getName() == name)) {
          return true;
        }
      } else if (brought.getName() == name) {
        return true;
      }
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleLibInImports( // NOLINT(misc-no-recursion)
    const String& name, const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name) || bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule* QatModule::getLib(const String& name, const utils::RequesterInfo& reqInfo) {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName() == name)) {
          return bMod;
        }
      } else if (brought.getName() == name) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name) || bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getLib(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openLib(const String& name, const String& filename, const utils::VisibilityInfo& visib_info,
                        llvm::LLVMContext& ctx) {
  if (!hasLib(name)) {
    addSubmodule(name, filename, ModuleType::lib, visib_info, ctx);
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

bool QatModule::hasBroughtBox(const String& name) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName() == name)) {
          return true;
        }
      } else if (brought.getName() == name) {
        return bMod;
      }
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleBoxInImports(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name) || bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule* QatModule::getBox(const String& name, const utils::RequesterInfo& reqInfo) {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) || (brought.getName() == name)) {
          return bMod;
        }
      } else if (brought.getName() == name) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name) || bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getBox(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openBox(const String& _name, Maybe<utils::VisibilityInfo> visib_info) {
  SHOW("Opening box: " << _name)
  if (hasBox(_name)) {
    for (auto* sub : submodules) {
      if (sub->moduleType == ModuleType::box) {
        if (sub->getName() == _name) {
          active = sub;
          break;
        }
      }
    }
  } else {
    addSubmodule(_name, filePath.string(), ModuleType::box, visib_info.value(), llvmModule->getContext());
  }
}

void QatModule::closeBox() { closeSubmodule(); }

bool QatModule::hasBroughtModule(const String& name) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (!brought.isNamed()) {
      if (bMod->shouldPrefixName() && (bMod->getName() == name)) {
        SHOW("Found brought module with name")
        return true;
      }
    } else if (brought.getName() == name) {
      SHOW("Found named brought module")
      SHOW("Brought module " << bMod)
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleBroughtModuleInImports(const String&               name,
                                                                  const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBroughtModule(name) || bMod->hasAccessibleBroughtModuleInImports(name, reqInfo).first) {
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

QatModule* QatModule::getBroughtModule(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (!brought.isNamed()) {
      if (bMod->shouldPrefixName() && (bMod->getName() == name)) {
        SHOW("Found brought module with name")
        return bMod;
      } else if (!bMod->shouldPrefixName()) {
        if (bMod->hasBroughtModule(name) || bMod->hasAccessibleBroughtModuleInImports(name, reqInfo).first) {
          SHOW("Found brought module in unnamed module")
          if (bMod->getBroughtModule(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getBroughtModule(name, reqInfo);
          }
        }
      }
    } else if (brought.getName() == name) {
      SHOW("Found named brought module")
      SHOW("Brought module " << bMod)
      return bMod;
    }
  }
  return nullptr;
}

void QatModule::bringModule(QatModule* other, const utils::VisibilityInfo& _visibility) {
  broughtModules.push_back(Brought<QatModule>(other, _visibility));
}

void QatModule::bringNamedModule(const String& _name, QatModule* other, const utils::VisibilityInfo& _visibility) {
  broughtModules.push_back(Brought<QatModule>(_name, other, _visibility));
}

bool QatModule::hasFunction(const String& name) const {
  SHOW("Function to be checked: " << name)
  SHOW("This pointer: " << this)
  SHOW("Function count: " << functions.size())
  for (auto* function : functions) {
    SHOW("Function in module: " << function)
    if (function->getName() == name) {
      SHOW("Found function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtFunction(const String& name) const {
  for (const auto& brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleFunctionInImports(const String&               name,
                                                             const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        SHOW("Checking brought function " << name << " in brought module " << bMod->getName() << " from file "
                                          << bMod->getFilePath())
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name) ||
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

Function* QatModule::getFunction(const String& name, const utils::RequesterInfo& reqInfo) {
  for (auto* function : functions) {
    if (function->getName() == name) {
      return function;
    }
  }
  for (const auto& brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName() == name) {
        return bFn;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name) ||
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

// TEMPLATE FUNCTION

bool QatModule::hasTemplateFunction(const String& name) const {
  SHOW("Template Function to be checked: " << name)
  SHOW("Template Function count: " << functions.size())
  for (auto* function : templateFunctions) {
    if (function->getName() == name) {
      SHOW("Found template function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool QatModule::hasBroughtTemplateFunction(const String& name) const {
  for (const auto& brought : broughtTemplateFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTemplateFunctionInImports(const String&               name,
                                                                     const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTemplateFunction(name) || bMod->hasBroughtTemplateFunction(name) ||
            bMod->hasAccessibleTemplateFunctionInImports(name, reqInfo).first) {
          if (bMod->getTemplateFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

TemplateFunction* QatModule::getTemplateFunction(const String& name, const utils::RequesterInfo& reqInfo) {
  for (auto* function : templateFunctions) {
    if (function->getName() == name) {
      return function;
    }
  }
  for (const auto& brought : broughtTemplateFunctions) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName() == name) {
        return bFn;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTemplateFunction(name) || bMod->hasBroughtTemplateFunction(name) ||
            bMod->hasAccessibleTemplateFunctionInImports(name, reqInfo).first) {
          if (bMod->getTemplateFunction(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getTemplateFunction(name, reqInfo);
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
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtCoreType(const String& name) const {
  for (const auto& brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto* cType = brought.get();
      if (cType->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleCoreTypeInImports(const String&               name,
                                                             const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name) ||
                                        bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first)) {
        if (bMod->getCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

CoreType* QatModule::getCoreType(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (auto* coreType : coreTypes) {
    if (coreType->getName() == name) {
      return coreType;
    }
  }
  for (const auto& brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto* coreType = brought.get();
      if (coreType->getName() == name) {
        return coreType;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name) ||
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
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtMixType(const String& name) const {
  for (const auto& brought : broughtMixTypes) {
    if (!brought.isNamed()) {
      auto* uType = brought.get();
      if (uType->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleMixTypeInImports(const String&               name,
                                                            const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasMixType(name) || bMod->hasBroughtMixType(name) ||
                                        bMod->hasAccessibleMixTypeInImports(name, reqInfo).first)) {
        if (bMod->getMixType(name, reqInfo)->isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

MixType* QatModule::getMixType(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (auto* mixTy : mixTypes) {
    if (mixTy->getName() == name) {
      return mixTy;
    }
  }
  for (const auto& brought : broughtMixTypes) {
    if (!brought.isNamed()) {
      auto* mixTy = brought.get();
      if (mixTy->getName() == name) {
        return mixTy;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasMixType(name) || bMod->hasBroughtMixType(name) ||
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
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtChoiceType(const String& name) const {
  for (const auto& brought : broughtChoiceTypes) {
    if (!brought.isNamed()) {
      auto* uType = brought.get();
      if (uType->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleChoiceTypeInImports(const String&               name,
                                                               const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasChoiceType(name) || bMod->hasBroughtChoiceType(name) ||
                                        bMod->hasAccessibleChoiceTypeInImports(name, reqInfo).first)) {
        if (bMod->getChoiceType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

ChoiceType* QatModule::getChoiceType(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (auto* choiceTy : choiceTypes) {
    if (choiceTy->getName() == name) {
      return choiceTy;
    }
  }
  for (const auto& brought : broughtChoiceTypes) {
    if (!brought.isNamed()) {
      auto* choiceTy = brought.get();
      if (choiceTy->getName() == name) {
        return choiceTy;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasChoiceType(name) || bMod->hasBroughtChoiceType(name) ||
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

// TEMPLATE CORE TYPE

bool QatModule::hasTemplateCoreType(const String& name) const {
  for (auto* tempCTy : templateCoreTypes) {
    SHOW("Template core type: " << tempCTy->getName())
    if (tempCTy->getName() == name) {
      SHOW("Found template core type")
      return true;
    }
  }
  SHOW("No template core types named " + name + " found")
  return false;
}

bool QatModule::hasBroughtTemplateCoreType(const String& name) const {
  for (const auto& brought : broughtTemplateCoreTypes) {
    if (!brought.isNamed()) {
      auto* bFn = brought.get();
      if (bFn->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTemplateCoreTypeInImports(const String&               name,
                                                                     const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTemplateCoreType(name) || bMod->hasBroughtTemplateCoreType(name) ||
            bMod->hasAccessibleTemplateCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getTemplateCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

TemplateCoreType* QatModule::getTemplateCoreType(const String& name, const utils::RequesterInfo& reqInfo) {
  for (auto* tempCore : templateCoreTypes) {
    if (tempCore->getName() == name) {
      return tempCore;
    }
  }
  for (const auto& brought : broughtTemplateCoreTypes) {
    if (!brought.isNamed()) {
      auto* bCTy = brought.get();
      if (bCTy->getName() == name) {
        return bCTy;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTemplateCoreType(name) || bMod->hasBroughtTemplateCoreType(name) ||
            bMod->hasAccessibleTemplateCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getTemplateCoreType(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
            return bMod->getTemplateCoreType(name, reqInfo);
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
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtTypeDef(const String& name) const {
  for (const auto& brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto* tDef = brought.get();
      if (tDef->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTypeDefInImports(const String&               name,
                                                            const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name) ||
                                        bMod->hasAccessibleTypeDefInImports(name, reqInfo).first)) {
        if (bMod->getTypeDef(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

DefinitionType* QatModule::getTypeDef(const String& name, const utils::RequesterInfo& reqInfo) const {
  for (auto* tDef : typeDefs) {
    if (tDef->getName() == name) {
      return tDef;
    }
  }
  for (const auto& brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto* tDef = brought.get();
      if (tDef->getName() == name) {
        return tDef;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name) ||
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

// GLOBAL ENTITY

bool QatModule::hasGlobalEntity(const String& name) const {
  for (auto* entity : globalEntities) {
    if (entity->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtGlobalEntity(const String& name) const {
  for (const auto& brought : broughtGlobalEntities) {
    if (!brought.isNamed()) {
      auto* bGlobal = brought.get();
      if (bGlobal->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGlobalEntityInImports(const String&               name,
                                                                 const utils::RequesterInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name) ||
                                        bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

GlobalEntity* QatModule::getGlobalEntity(const String&               name, // NOLINT(misc-no-recursion)
                                         const utils::RequesterInfo& reqInfo) const {
  for (auto* ent : globalEntities) {
    if (ent->getName() == name) {
      return ent;
    }
  }
  for (const auto& brought : broughtGlobalEntities) {
    auto* bGlobal = brought.get();
    if (!brought.isNamed()) {
      if (bGlobal->getName() == name) {
        return bGlobal;
      }
    } else if (brought.getName() == name) {
      return bGlobal;
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.isNamed()) {
      auto* bMod = brought.get();
      if (!bMod->shouldPrefixName() && (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name) ||
                                        bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)->getVisibility().isAccessible(reqInfo)) {
          return bMod->getGlobalEntity(name, reqInfo);
        }
      }
    }
  }
  return nullptr;
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
  SHOW("Creating modules via nodes" << filePath.string())
  ctx->mod = this;
  for (auto* node : nodes) {
    node->createModule(ctx);
  }
  for (auto* sub : submodules) {
    sub->createModules(ctx);
  }
}

void QatModule::handleBrings(IR::Context* ctx) {
  ctx->mod = this;
  for (auto* node : nodes) {
    node->handleBrings(ctx);
  }
  for (auto* sub : submodules) {
    sub->handleBrings(ctx);
  }
}

void QatModule::defineTypes(IR::Context* ctx) {
  SHOW("Defining types")
  ctx->mod = this;
  for (auto& node : nodes) {
    node->defineType(ctx);
    if ((node->nodeType() == ast::NodeType::defineCoreType) && (((ast::DefineCoreType*)node)->isTemplate())) {
      node = new ast::HolderNode(node);
    }
  }
  for (auto* sub : submodules) {
    sub->defineTypes(ctx);
  }
}

void QatModule::defineNodes(IR::Context* ctx) {
  SHOW("Defining nodes")
  ctx->mod = this;
  for (auto& node : nodes) {
    node->define(ctx);
    if ((node->nodeType() == ast::NodeType::functionDefinition) && (((ast::FunctionDefinition*)node)->isTemplate())) {
      node = new ast::HolderNode(node);
    } else if ((node->nodeType() == ast::NodeType::functionPrototype) &&
               (((ast::FunctionPrototype*)node)->isTemplate())) {
      node = new ast::HolderNode(node);
    }
  }
  for (auto* sub : submodules) {
    sub->defineNodes(ctx);
  }
}

void QatModule::emitNodes(IR::Context* ctx) {
  if (!isEmitted) {
    ctx->mod = this;
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
      SHOW("Emission complete, linking LLVM modules")
    }
    isEmitted = true;
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
    fs::create_directory(llPath.parent_path(), errorCode);
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
      SHOW("Error could not create directory")
    }
  }
}

void QatModule::compileToObject(IR::Context* ctx) {
  if (!isCompiledToObject) {
    SHOW("Compiling Module `" << name << "` from file " << filePath.string())
    String compileCommand("clang -fPIC -c ");
    if (moduleInfo.linkPthread) {
      compileCommand += "-pthread ";
    }
    auto* cfg = cli::Config::get();
    compileCommand.append("--target=").append(cfg->getTargetTriple()).append(" ");
    if (cfg->hasSysroot()) {
      compileCommand.append("--sysroot=").append(cfg->getSysroot()).append(" ");
      //-Wl,--import-memory
      // compileCommand.append(" -nostartfiles -Wl,--no-entry ");
    }
    for (auto* sub : submodules) {
      sub->compileToObject(ctx);
    }
    // FIXME - Also link modules of other brought entities
    objectFilePath = (cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) / "object" /
                     filePath.lexically_relative(basePath).replace_filename(getWritableName().append(".o"));
    SHOW("Creating all folders in object file output path: " << objectFilePath)
    fs::create_directory(objectFilePath.parent_path());
    compileCommand.append(llPath.string()).append(" -o ").append(objectFilePath.string());
    SHOW("Command is: " << compileCommand)
    if (system(compileCommand.c_str())) {
      ctx->writeJsonResult(false);
      ctx->Error("Could not compile the LLVM file", {filePath, {0u, 0u}, {0u, 0u}});
    }
    isCompiledToObject = true;
  }
}

void QatModule::bundleLibs(IR::Context* ctx) {
  if (!isBundled) {
    for (auto* sub : submodules) {
      sub->bundleLibs(ctx);
    }
    auto*  cfg = cli::Config::get();
    String cmdRem("-fPIC ");
    if (moduleInfo.linkPthread) {
      cmdRem.append("-pthread ");
    }
    cmdRem.append(" --target=").append(cfg->getTargetTriple()).append(" ");
    if (cfg->hasSysroot()) {
      cmdRem.append("--sysroot=").append(cfg->getSysroot()).append(" ");
      // -Wl,--import-memory
      // cmdRem.append(" -nostartfiles -Wl,--no-entry ");
    }
    cmdRem.append(objectFilePath.string()).append(" ");
    for (auto* sub : submodules) {
      cmdRem.append(sub->objectFilePath.string()).append(" ");
    }
    SHOW("Added ll paths of all submodules")
    for (const auto& bMod : broughtModules) {
      SHOW("Brought module: " << bMod.get())
      SHOW("Brought module name: " << bMod.get()->name)
      SHOW("Brought module path: " << bMod.get()->objectFilePath)
      cmdRem.append(bMod.get()->objectFilePath.string()).append(" ");
    }
    SHOW("Added ll paths of all brought modules")
    if (hasMain) {
      auto outPath = ((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                      filePath.lexically_relative(basePath)
                          .replace_filename(moduleInfo.outputName.value_or(name))
                          .replace_extension(""))
                         .string()
                         .append(" ");
      if (system(String("clang -o ").append(outPath).append(cmdRem).c_str())) {
        ctx->Error("Compiling executable failed", {"", {0u, 0u}, {0u, 0u}});
      }
    } else {
      if (moduleInfo.staticBuild) {
        auto outPath = ((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                        filePath.lexically_relative(basePath).replace_filename(
                            moduleInfo.outputName.value_or(getWritableName()).append(".a")))
                           .string()
                           .append(" ");
        if (system(String("ar r ").append(outPath).append(cmdRem).c_str())) {
          ctx->Error("Static compilation of module " + ctx->highlightError(filePath.string()) + " failed",
                     {"", {0u, 0u}, {0u, 0u}});
        }
      }
      if (moduleInfo.sharedBuild) {
        auto outPath = ((cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                        filePath.lexically_relative(basePath).replace_filename(
                            moduleInfo.outputName.value_or(getWritableName()).append(".so")))
                           .string()
                           .append(" ");
        if (system(String("clang ")
                       .append(hasMain ? "-fuse-ld=lld" : "")
                       .append(" -shared -o ")
                       .append(outPath)
                       .append(cmdRem)
                       .c_str())) {
          ctx->Error("Static compilation of module " + ctx->highlightError(filePath.string()) + " failed",
                     {"", {0u, 0u}, {0u, 0u}});
        }
      }
    }
    isBundled = true;
  }
}

bool QatModule::hasMainFn() const { return hasMain; }

void QatModule::setHasMainFn() { hasMain = true; }

fs::path QatModule::getResolvedOutputPath(const String& extension) const {
  auto* config = cli::Config::get();
  auto  fPath  = filePath;
  auto  out    = fPath.replace_extension(extension);
  if (config->hasOutputPath()) {
    out = (config->getOutputPath() / filePath.lexically_relative(basePath).remove_filename());
    fs::create_directories(out);
    out = out / fPath.filename();
  }
  return out;
}

void QatModule::exportJsonFromAST() const {
  if (moduleType == ModuleType::file) {
    auto           result = Json();
    Vec<JsonValue> contents;
    for (auto* node : nodes) {
      contents.push_back(node->toJson());
    }
    result["contents"] = contents;
    std::fstream jsonStream;
    jsonStream.open(getResolvedOutputPath(".json"), std::ios_base::out);
    if (jsonStream.is_open()) {
      jsonStream << result;
      jsonStream.close();
    } else {
      std::cout << "File could not be opened for writing. Outputting the "
                   "JSON representation of AST...\n\n";
      std::cout << result << "\n";
    }
  }
}

llvm::Module* QatModule::getLLVMModule() const { return llvmModule; }

void QatModule::linkNative(NativeUnit nval) {
  // FIXME - Use integer widths according to the specification and not the same on all platforms
  llvm::LLVMContext& ctx = llvmModule->getContext();
  switch (nval) {
    case NativeUnit::printf: {
      if (!llvmModule->getFunction("printf")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), {llvm::Type::getInt8Ty(ctx)->getPointerTo()}, true),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "printf", llvmModule);
      }
      break;
    }
    case NativeUnit::malloc: {
      if (!llvmModule->getFunction("malloc")) {
        SHOW("Creating malloc function")
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt8Ty(ctx)->getPointerTo(), {llvm::Type::getInt64Ty(ctx)}, false),
            llvm::GlobalValue::ExternalWeakLinkage, "malloc", llvmModule);
      }
      break;
    }
    case NativeUnit::free: {
      if (!llvmModule->getFunction("free")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {llvm::Type::getInt8Ty(ctx)->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "free", llvmModule);
      }
      break;
    }
    case NativeUnit::realloc: {
      if (!llvmModule->getFunction("realloc")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt8Ty(ctx)->getPointerTo(),
                                    {llvm::Type::getInt8Ty(ctx)->getPointerTo(), llvm::Type::getInt64Ty(ctx)}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "realloc", llvmModule);
      }
      break;
    }
    case NativeUnit::pthreadCreate: {
      if (!llvmModule->getFunction("pthread_create")) {
        llvm::Type* pthreadPtrTy = llvm::Type::getInt64Ty(ctx)->getPointerTo();
        llvm::Type* voidPtrTy    = llvm::Type::getInt8Ty(ctx)->getPointerTo();
        auto*       pthreadFnTy  = llvm::FunctionType::get(voidPtrTy, {voidPtrTy}, false);
        if (!llvm::StructType::getTypeByName(ctx, "pthread_attr_t")) {
          llvm::StructType::create(ctx,
                                   {llvm::Type::getInt64Ty(ctx), llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), 48u)},
                                   "pthread_attr_t");
        }
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx),
                                    {pthreadPtrTy,
                                     llvm::StructType::getTypeByName(ctx, "pthread_attr_t")->getPointerTo(),
                                     pthreadFnTy->getPointerTo(), voidPtrTy},
                                    false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_create", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadJoin: {
      if (!llvmModule->getFunction("pthread_join")) {
        llvm::Type* pthreadTy = llvm::Type::getInt64Ty(ctx);
        llvm::Type* voidPtrTy = llvm::Type::getInt8Ty(ctx)->getPointerTo();
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), {pthreadTy, voidPtrTy->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_join", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadExit: {
      if (!llvmModule->getFunction("pthread_exit")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {llvm::Type::getInt8Ty(ctx)->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_exit", llvmModule);
        moduleInfo.linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadAttrInit: {
      if (!llvmModule->getFunction("pthread_attr_init")) {
        if (!llvm::StructType::getTypeByName(ctx, "pthread_attr_t")) {
          llvm::StructType::create(ctx,
                                   {llvm::Type::getInt64Ty(ctx), llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), 48u)},
                                   "pthread_attr_t");
        }
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx),
                                    {llvm::StructType::getTypeByName(ctx, "pthread_attr_t")->getPointerTo()}, false),
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
