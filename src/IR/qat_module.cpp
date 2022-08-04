#include "./qat_module.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"
#include "llvm/Config/llvm-config.h"
#include <filesystem>
#include <fstream>

namespace qat::IR {

QatModule::QatModule(const String &_name, const fs::path &_filepath,
                     const fs::path &_basePath, ModuleType _type,
                     const utils::VisibilityInfo &_visibility)
    : name(_name), moduleType(_type), filePath(_filepath), basePath(_basePath),
      visibility(_visibility), parent(nullptr), active(nullptr) {}

QatModule::~QatModule() = default;

QatModule *QatModule::Create(const String &name, const fs::path &filepath,
                             const fs::path &basePath, ModuleType type,
                             const utils::VisibilityInfo &visib_info) {
  return new QatModule(name, filepath, basePath, type, visib_info);
}

const utils::VisibilityInfo &QatModule::getVisibility() const {
  return visibility;
}

QatModule *QatModule::getActive() { // NOLINT(misc-no-recursion)
  if (active) {
    return active->getActive();
  } else {
    return this;
  }
}

QatModule *QatModule::getParentFile() { // NOLINT(misc-no-recursion)
  if (moduleType == ModuleType::file) {
    return this;
  } else {
    if (parent) {
      return parent->getParentFile();
    }
  }
}

Pair<unsigned, String> QatModule::resolveNthParent(const String &name) const {
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

QatModule *QatModule::getNthParent(u32 n) { // NOLINT(misc-no-recursion)
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

Function *QatModule::createFunction(const String &name, QatType *returnType,
                                    const bool          isReturnTypeVariable,
                                    const bool          isAsync,
                                    const Vec<Argument> args,
                                    const bool          isVariadic,
                                    const utils::FileRange      &fileRange,
                                    const utils::VisibilityInfo &visibility) {
  SHOW("Creating IR function")
  auto fn = Function::Create(this, name, returnType, isReturnTypeVariable,
                             isAsync, args, isVariadic, fileRange, visibility);
  SHOW("Created function")
  functions.push_back(fn);
  return fn;
}

QatModule *
QatModule::CreateSubmodule(QatModule *parent, fs::path filepath,
                           fs::path basePath, String name, ModuleType type,
                           const utils::VisibilityInfo &visibilityInfo) {
  auto sub = new QatModule(std::move(name), std::move(filepath),
                           std::move(basePath), type, visibilityInfo);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  return sub;
}

QatModule *QatModule::CreateFile(QatModule *parent, fs::path filepath,
                                 fs::path basePath, String name,
                                 Vec<ast::Node *>      nodes,
                                 utils::VisibilityInfo visibilityInfo) {
  auto sub =
      new QatModule(name, filepath, basePath, ModuleType::file, visibilityInfo);
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
    return parent->getFullName() +
           (shouldPrefixName()
                ? ((parent->shouldPrefixName() ? ":" : "") + name)
                : "");
  } else {
    return shouldPrefixName() ? name : "";
  }
}

String QatModule::getFullNameWithChild(const String &child) const {
  if (!getFullName().empty()) {
    return getFullName() + ":" + child;
  } else {
    return child;
  }
}

bool QatModule::shouldPrefixName() const {
  return ((moduleType != ModuleType::file) &&
          (moduleType != ModuleType::folder));
}

bool QatModule::isSubmodule() const { return parent != nullptr; }

void QatModule::addSubmodule(const String &name, const String &filename,
                             ModuleType                   type,
                             const utils::VisibilityInfo &visib_info) {
  active = CreateSubmodule(active, name, filename, basePath.string(), type,
                           visib_info);
}

void QatModule::closeSubmodule() { active = nullptr; }

bool QatModule::hasLib(const String &name) const {
  for (auto *sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtLib(const String &name) const {
  for (auto brought : broughtModules) {
    auto *bMod = brought.get();
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

Pair<bool, String>
QatModule::hasAccessibleLibInImports( // NOLINT(misc-no-recursion)
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name) ||
            bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule *QatModule::getLib(const String               &name,
                             const utils::RequesterInfo &reqInfo) {
  // FIXME
}

void QatModule::openLib(const String &name, const String &filename,
                        const utils::VisibilityInfo &visib_info) {
  addSubmodule(name, filename, ModuleType::lib, visib_info);
}

void QatModule::closeLib() { closeSubmodule(); }

bool QatModule::hasBox(const String &name) const {
  for (auto *sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtBox(const String &name) const {
  for (auto brought : broughtModules) {
    auto *bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) ||
            (brought.getName() == name)) {
          return true;
        }
      } else if (brought.getName() == name) {
        return bMod;
      }
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleBoxInImports(
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

QatModule *QatModule::getBox(const String               &name,
                             const utils::RequesterInfo &reqInfo) {
  for (auto *sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (auto brought : broughtModules) {
    auto *bMod = brought.get();
    if (bMod->moduleType == ModuleType::box) {
      if (!brought.isNamed()) {
        if ((bMod->shouldPrefixName() && (bMod->getName() == name)) ||
            (brought.getName() == name)) {
          return bMod;
        }
      } else if (brought.getName() == name) {
        return bMod;
      }
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return bMod->getBox(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void QatModule::openBox(const String                &name,
                        Maybe<utils::VisibilityInfo> visib_info) {
  // FIXME - Change this
  // for (auto sub : submodules) {
  //   if (sub->name == name) {
  //     if (sub->type == ModuleType::box) {
  //       active = sub;
  //       active->add_submodule(name, ModuleType::boxAddition, visib_info);
  //     } else if (sub->type == ModuleType::lib) {
  //     }
  //   }
  // }
}

void QatModule::closeBox() { closeSubmodule(); }

bool QatModule::hasFunction(const String &name) const {
  for (auto *function : functions) {
    if (function->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtFunction(const String &name) const {
  for (auto brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto *bFn = brought.get();
      if (bFn->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleFunctionInImports(
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name) ||
            bMod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          if (bMod->getFunction(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

Function *QatModule::getFunction(const String               &name,
                                 const utils::RequesterInfo &reqInfo) {
  for (auto *function : functions) {
    if (function->getName() == name) {
      return function;
    }
  }
  for (auto brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto *bFn = brought.get();
      if (bFn->getName() == name) {
        return bFn;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name) ||
            bMod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          if (bMod->getFunction(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return bMod->getFunction(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

bool QatModule::hasCoreType(const String &name) const {
  for (auto *typ : coreTypes) {
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtCoreType(const String &name) const {
  for (auto brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto *cType = brought.get();
      if (cType->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleCoreTypeInImports(
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name) ||
           bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first)) {
        if (bMod->getCoreType(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

CoreType *QatModule::getCoreType(const String               &name,
                                 const utils::RequesterInfo &reqInfo) const {
  for (auto *coreType : coreTypes) {
    if (coreType->getName() == name) {
      return coreType;
    }
  }
  for (auto brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto *coreType = brought.get();
      if (coreType->getName() == name) {
        return coreType;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name) ||
            bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first) {
          if (bMod->getCoreType(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return bMod->getCoreType(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

bool QatModule::hasGlobalEntity(const String &name) const {
  for (auto *entity : globalEntities) {
    if (entity->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtGlobalEntity(const String &name) const {
  for (auto brought : broughtGlobalEntities) {
    if (!brought.isNamed()) {
      auto *bGlobal = brought.get();
      if (bGlobal->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleGlobalEntityInImports(
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name) ||
           bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

GlobalEntity *QatModule::createGlobalEntity() { return nullptr; }

GlobalEntity *
QatModule::getGlobalEntity(const String &name, // NOLINT(misc-no-recursion)
                           const utils::RequesterInfo &reqInfo) const {
  for (auto *ent : globalEntities) {
    if (ent->getName() == name) {
      return ent;
    }
  }
  for (auto brought : broughtGlobalEntities) {
    auto *bGlobal = brought.get();
    if (!brought.isNamed()) {
      if (bGlobal->getName() == name) {
        return bGlobal;
      }
    } else if (brought.getName() == name) {
      return bGlobal;
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasGlobalEntity(name) || bMod->hasBroughtGlobalEntity(name) ||
           bMod->hasAccessibleGlobalEntityInImports(name, reqInfo).first)) {
        if (bMod->getGlobalEntity(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return bMod->getGlobalEntity(name, reqInfo);
        }
      }
    }
  }
  return nullptr;
}

bool QatModule::areNodesEmitted() const { return isEmitted; }

void QatModule::emitNodes(IR::Context *ctx) const {
  if (!isEmitted) {
    for (auto *node : nodes) {
      node->emit(ctx);
    }
    isEmitted = true;
  }
}

fs::path QatModule::getResolvedOutputPath(const String &extension) const {
  auto *config = cli::Config::get();
  auto  fPath  = filePath;
  auto  out    = fPath.replace_extension(extension);
  if (config->hasOutputPath()) {
    out = (config->getOutputPath() /
           filePath.lexically_relative(basePath).remove_filename());
    fs::create_directories(out);
    out = out / fPath.filename();
  }
  return out;
}

void QatModule::exportJsonFromAST() const {
  if (moduleType == ModuleType::file) {
    auto                result = nuo::Json();
    Vec<nuo::JsonValue> contents;
    for (auto *node : nodes) {
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

void QatModule::addFunctionToLink(Function *fun) {
  functionsToLink.push_back(fun);
}

void QatModule::defineLLVM(llvmHelper &help) const {
  if (!llvmModule) {
    llvmModule = new llvm::Module(getFullName(), help.llctx);
    llvmModule->setSourceFileName(filePath.string());
    llvmModule->setCodeModel(llvm::CodeModel::Small);
    llvmModule->setTargetTriple(LLVM_HOST_TRIPLE);
  }
  // TODO - Implement
}

void QatModule::defineCPP(cpp::File &file) const { // NOLINT(misc-no-recursion)
  if (moduleType == ModuleType::lib) {
    for (auto *mod : submodules) {
      auto resolvedPath = mod->filePath.lexically_relative(basePath);
      if (mod->moduleType == ModuleType::file) {
        auto headerFile =
            cpp::File::Header(resolvedPath.replace_extension(".hpp").string());
        mod->defineCPP(headerFile);
      }
    }
  } else if (moduleType == ModuleType::box) {
    file << ("namespace " + name + " {\n\n");
    for (auto const &broughtModule : broughtModules) {
      auto imPath =
          broughtModule.get()->getParentFile()->getResolvedOutputPath(".hpp");
    }
    for (auto *coreType : coreTypes) {
      coreType->defineCPP(file);
    }
    for (auto *globalEntity : globalEntities) {
      globalEntity->defineCPP(file);
    }
    for (auto *function : functions) {
      function->defineCPP(file);
    }
    for (auto *submodule : submodules) {
      submodule->defineCPP(file);
    }
    file << "\n} // namespace " << name << "\n";
  }
}

nuo::Json QatModule::toJson() const {
  // FIXME - Change
  return nuo::Json()._("name", name);
}

llvm::Module &QatModule::getLLVMModule(llvmHelper &) const {
  return *llvmModule;
}

void QatModule::emitCPP(cpp::File &file) const {
  // TODO - Implement
}

} // namespace qat::IR