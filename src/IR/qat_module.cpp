#include "./qat_module.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"
#include <filesystem>
#include <fstream>

namespace qat::IR {

QatModule::QatModule(std::string _name, fs::path _filepath, fs::path _basePath,
                     ModuleType _type, utils::VisibilityInfo _visibility)
    : parent(nullptr), name(_name), moduleType(_type), filePath(_filepath),
      basePath(_basePath), visibility(_visibility), active(nullptr) {}

QatModule::~QatModule() {}

QatModule *QatModule::Create(std::string name, fs::path filepath,
                             fs::path basePath, ModuleType type,
                             utils::VisibilityInfo visib_info) {
  return new QatModule(name, filepath, basePath, type, visib_info);
}

const utils::VisibilityInfo &QatModule::getVisibility() const {
  return visibility;
}

QatModule *QatModule::getActive() {
  if (active) {
    return active->getActive();
  } else {
    return this;
  }
}

std::pair<unsigned, std::string>
QatModule::resolveNthParent(const std::string name) const {
  unsigned result = 0;
  if (name.find("..:") == 0) {
    unsigned i;
    for (i = 0; i < name.length(); i++) {
      if (name.find("..:", i) == i) {
        result++;
        i += 2;
      } else {
        break;
      }
    }
    return std::pair(result, name.substr(i));
  }
  return std::pair(result, name);
}

bool QatModule::hasNthParent(unsigned int n) const {
  if (n == 1) {
    return (parent != nullptr);
  } else if (n > 1) {
    return hasNthParent(n - 1);
  } else {
    return true;
  }
}

QatModule *QatModule::getNthParent(unsigned int n) {
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

Function *QatModule::createFunction(const std::string name, QatType *returnType,
                                    const bool isReturnTypeVariable,
                                    const bool isAsync,
                                    const std::vector<Argument> args,
                                    const bool isVariadic,
                                    const utils::FileRange fileRange,
                                    const utils::VisibilityInfo visibility) {
  SHOW("Creating IR function")
  auto fn = Function::Create(this, name, returnType, isReturnTypeVariable,
                             isAsync, args, isVariadic, fileRange, visibility);
  SHOW("Created function")
  functions.push_back(fn);
  return fn;
}

QatModule *QatModule::CreateSubmodule(QatModule *parent, fs::path filepath,
                                      fs::path basePath, std::string name,
                                      ModuleType type,
                                      utils::VisibilityInfo visib_info) {
  auto sub = new QatModule(name, filepath, basePath, type, visib_info);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  return sub;
}

QatModule *QatModule::CreateFile(QatModule *parent, fs::path filepath,
                                 fs::path basePath, std::string name,
                                 std::vector<ast::Node *> nodes,
                                 utils::VisibilityInfo visibility) {
  auto sub =
      new QatModule(name, filepath, basePath, ModuleType::file, visibility);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  sub->nodes = std::move(nodes);
  return sub;
}

std::string QatModule::getName() const { return name; }

std::string QatModule::getFullName() const {
  if (parent) {
    return parent->getFullName() + (shouldPrefixName() ? (":" + name) : "");
  } else {
    return shouldPrefixName() ? name : "";
  }
}

std::string QatModule::getFullNameWithChild(std::string child) const {
  if (getFullName() != "") {
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

void QatModule::addSubmodule(std::string name, std::string filename,
                             ModuleType type,
                             utils::VisibilityInfo visib_info) {
  active = CreateSubmodule(active, name, filename, basePath, type, visib_info);
}

void QatModule::closeSubmodule() { active = nullptr; }

bool QatModule::hasLib(const std::string name) const {
  for (auto sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtLib(const std::string name) const {
  for (auto brought : broughtModules) {
    auto bMod = brought.get();
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

std::pair<bool, std::string> QatModule::hasAccessibleLibInImports(
    const std::string name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasLib(name) || bMod->hasBroughtLib(name) ||
            bMod->hasAccessibleLibInImports(name, reqInfo).first) {
          if (bMod->getLib(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return std::pair(true, bMod->filePath);
          }
        }
      }
    }
  }
  return std::pair(false, "");
}

QatModule *QatModule::getLib(const std::string name,
                             const utils::RequesterInfo &reqInfo) {
  // FIXME
}

void QatModule::openLib(std::string name, std::string filename,
                        utils::VisibilityInfo visib_info) {
  addSubmodule(name, filename, ModuleType::lib, visib_info);
}

void QatModule::closeLib() { closeSubmodule(); }

bool QatModule::hasBox(const std::string name) const {
  for (auto sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtBox(const std::string name) const {
  for (auto brought : broughtModules) {
    auto bMod = brought.get();
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

std::pair<bool, std::string> QatModule::hasAccessibleBoxInImports(
    const std::string name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasBox(name) || bMod->hasBroughtBox(name) ||
            bMod->hasAccessibleBoxInImports(name, reqInfo).first) {
          if (bMod->getBox(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return std::pair(true, bMod->filePath);
          }
        }
      }
    }
  }
  return std::pair(false, "");
}

QatModule *QatModule::getBox(const std::string name,
                             const utils::RequesterInfo &reqInfo) {
  for (auto sub : submodules) {
    if ((sub->moduleType == ModuleType::box) && (sub->getName() == name)) {
      return sub;
    }
  }
  for (auto brought : broughtModules) {
    auto bMod = brought.get();
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
      auto bMod = brought.get();
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

void QatModule::openBox(std::string name,
                        std::optional<utils::VisibilityInfo> visib_info) {
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

bool QatModule::hasFunction(const std::string name) const {
  for (auto fn : functions) {
    if (fn->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtFunction(const std::string name) const {
  for (auto brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto bFn = brought.get();
      if (bFn->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

std::pair<bool, std::string> QatModule::hasAccessibleFunctionInImports(
    const std::string name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasFunction(name) || bMod->hasBroughtFunction(name) ||
            bMod->hasAccessibleFunctionInImports(name, reqInfo).first) {
          if (bMod->getFunction(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return std::pair(true, bMod->filePath);
          }
        }
      }
    }
  }
  return std::pair(false, "");
}

Function *QatModule::getFunction(const std::string name,
                                 const utils::RequesterInfo &reqInfo) {
  for (auto fn : functions) {
    if (fn->getName() == name) {
      return fn;
    }
  }
  for (auto brought : broughtFunctions) {
    if (!brought.isNamed()) {
      auto bFn = brought.get();
      if (bFn->getName() == name) {
        return bFn;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
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

bool QatModule::hasCoreType(const std::string name) const {
  for (auto typ : coreTypes) {
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtCoreType(const std::string name) const {
  for (auto brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto cType = brought.get();
      if (cType->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

std::pair<bool, std::string> QatModule::hasAccessibleCoreTypeInImports(
    const std::string name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
      if ((bMod->shouldPrefixName()
               ? false
               : (bMod->hasCoreType(name) || bMod->hasBroughtCoreType(name) ||
                  bMod->hasAccessibleCoreTypeInImports(name, reqInfo).first))) {
        if (bMod->getCoreType(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return std::pair(true, bMod->filePath);
        }
      }
    }
  }
  return std::pair(false, "");
}

CoreType *QatModule::getCoreType(const std::string name,
                                 const utils::RequesterInfo &reqInfo) const {
  for (auto ct : coreTypes) {
    if (ct->getName() == name) {
      return ct;
    }
  }
  for (auto brought : broughtCoreTypes) {
    if (!brought.isNamed()) {
      auto cType = brought.get();
      if (cType->getName() == name) {
        return cType;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
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

bool QatModule::hasGlobalEntity(const std::string name) const {
  for (auto ge : globalEntities) {
    if (ge->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtGlobalEntity(const std::string name) const {
  for (auto brought : broughtGlobalEntities) {
    if (!brought.isNamed()) {
      auto bGlobal = brought.get();
      if (bGlobal->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

std::pair<bool, std::string> QatModule::hasAccessibleGlobalEntityInImports(
    const std::string name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto bMod = brought.get();
      if ((bMod->shouldPrefixName()
               ? false
               : (bMod->hasGlobalEntity(name) ||
                  bMod->hasBroughtGlobalEntity(name) ||
                  bMod->hasAccessibleGlobalEntityInImports(name, reqInfo)
                      .first))) {
        if (bMod->getGlobalEntity(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return std::pair(true, bMod->filePath);
        }
      }
    }
  }
  return std::pair(false, "");
}

GlobalEntity *QatModule::createGlobalEntity() {}

GlobalEntity *
QatModule::getGlobalEntity(const std::string name,
                           const utils::RequesterInfo &reqInfo) const {
  for (auto ent : globalEntities) {
    if (ent->getName() == name) {
      return ent;
    }
  }
  for (auto brought : broughtGlobalEntities) {
    auto bGlobal = brought.get();
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
      auto bMod = brought.get();
      if ((bMod->shouldPrefixName()
               ? false
               : (bMod->hasGlobalEntity(name) ||
                  bMod->hasBroughtGlobalEntity(name) ||
                  bMod->hasAccessibleGlobalEntityInImports(name, reqInfo)
                      .first))) {
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

void QatModule::emitNodes(IR::Context *ctx) const {
  if (!isEmitted) {
    for (auto node : nodes) {
      node->emit(ctx);
    }
  }
}

void QatModule::exportJsonFromAST() const {
  if (moduleType == ModuleType::file) {
    auto result = nuo::Json();
    std::vector<nuo::JsonValue> contents;
    for (auto node : nodes) {
      contents.push_back(node->toJson());
    }
    result["contents"] = contents;
    std::fstream jsonStream;
    auto config = cli::Config::get();
    auto fPath = filePath;
    auto out = fPath.replace_extension(".json");
    if (config->hasOutputPath()) {
      out = (config->getOutputPath() /
             filePath.lexically_relative(basePath).remove_filename());
      fs::create_directories(out);
      out = out / fPath.filename().string();
    }
    jsonStream.open(out, std::ios_base::out);
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

} // namespace qat::IR