#include "./qat_module.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "member_function.hpp"
#include "types/float.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace qat::IR {

QatModule::QatModule(String _name, fs::path _filepath, fs::path _basePath,
                     ModuleType _type, const utils::VisibilityInfo &_visibility,
                     llvm::LLVMContext &ctx)
    : name(std::move(_name)), moduleType(_type), filePath(std::move(_filepath)),
      basePath(std::move(_basePath)), visibility(_visibility), parent(nullptr),
      active(nullptr) {
  llvmModule = new llvm::Module(getFullName(), ctx);
  llvmModule->setModuleIdentifier(getFullName());
  llvmModule->setSourceFileName(filePath.string());
  llvmModule->setCodeModel(llvm::CodeModel::Small);
  llvmModule->setTargetTriple(LLVM_HOST_TRIPLE);
}

QatModule::~QatModule() = default;

QatModule *QatModule::Create(const String &name, const fs::path &filepath,
                             const fs::path &basePath, ModuleType type,
                             const utils::VisibilityInfo &visib_info,
                             llvm::LLVMContext           &ctx) {
  return new QatModule(name, filepath, basePath, type, visib_info, ctx);
}

ModuleType QatModule::getModuleType() const { return moduleType; }

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
                                    bool isReturnTypeVariable, bool isAsync,
                                    Vec<Argument> args, bool isVariadic,
                                    const utils::FileRange         &fileRange,
                                    const utils::VisibilityInfo    &visibility,
                                    llvm::GlobalValue::LinkageTypes linkage,
                                    llvm::LLVMContext              &ctx) {
  SHOW("Creating IR function")
  auto fn =
      Function::Create(this, name, returnType, isReturnTypeVariable, isAsync,
                       std::move(args), isVariadic, fileRange, visibility, ctx);
  SHOW("Created function")
  functions.push_back(fn);
  return fn;
}

QatModule *
QatModule::CreateSubmodule(QatModule *parent, fs::path filepath,
                           fs::path basePath, String name, ModuleType type,
                           const utils::VisibilityInfo &visibilityInfo,
                           llvm::LLVMContext           &ctx) {
  auto sub = new QatModule(std::move(name), std::move(filepath),
                           std::move(basePath), type, visibilityInfo, ctx);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  return sub;
}

QatModule *QatModule::CreateFile(QatModule *parent, fs::path filepath,
                                 fs::path basePath, String name,
                                 Vec<String> content, Vec<ast::Node *> nodes,
                                 utils::VisibilityInfo visibilityInfo,
                                 llvm::LLVMContext    &ctx) {
  auto sub     = new QatModule(name, filepath, basePath, ModuleType::file,
                               visibilityInfo, ctx);
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

Function *QatModule::getGlobalInitialiser(IR::Context *ctx) {
  if (!globalInitialiser) {
    globalInitialiser = IR::Function::Create(
        this, "qat'global'initialiser", IR::VoidType::get(ctx->llctx), false,
        false, {}, false,
        utils::FileRange("", utils::FilePos{0u, 0u}, utils::FilePos{0u, 0u}),
        utils::VisibilityInfo::pub(), ctx->llctx);
    auto *entry = new IR::Block(globalInitialiser, nullptr);
    entry->setActive(ctx->builder);
  }
  return globalInitialiser;
}

bool QatModule::isSubmodule() const { return parent != nullptr; }

void QatModule::addSubmodule(const String &name, const String &filename,
                             ModuleType                   type,
                             const utils::VisibilityInfo &visib_info,
                             llvm::LLVMContext           &ctx) {
  active = CreateSubmodule(active, name, filename, basePath.string(), type,
                           visib_info, ctx);
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
                        const utils::VisibilityInfo &visib_info,
                        llvm::LLVMContext           &ctx) {
  if (!hasLib(name)) {
    addSubmodule(name, filename, ModuleType::lib, visib_info, ctx);
  }
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
  if (hasBox(name)) {
    for (auto *sub : submodules) {
      if (sub->moduleType == ModuleType::box) {
        if (sub->getName() == name) {
          active = sub;
          break;
        }
      }
    }
  } else {
    addSubmodule(name, filePath, ModuleType::box, visib_info.value(),
                 llvmModule->getContext());
  }
}

void QatModule::closeBox() { closeSubmodule(); }

bool QatModule::hasFunction(const String &name) const {
  SHOW("Function count: " << functions.size())
  for (auto *function : functions) {
    if (function->getName() == name) {
      SHOW("Found function")
      return true;
    }
  }
  SHOW("No functions named " + name + " found")
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

// CORE TYPE

bool QatModule::hasCoreType(const String &name) const {
  SHOW("CoreType count: " << coreTypes.size())
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

// TYPEDEF

bool QatModule::hasTypeDef(const String &name) const {
  SHOW("TypeDef count: " << typeDefs.size())
  for (auto *typ : typeDefs) {
    if (typ->getName() == name) {
      return true;
    }
  }
  return false;
}

bool QatModule::hasBroughtTypeDef(const String &name) const {
  for (auto brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto *tDef = brought.get();
      if (tDef->getName() == name) {
        return true;
      }
    } else if (brought.getName() == name) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> QatModule::hasAccessibleTypeDefInImports(
    const String &name, const utils::RequesterInfo &reqInfo) const {
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName() &&
          (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name) ||
           bMod->hasAccessibleTypeDefInImports(name, reqInfo).first)) {
        if (bMod->getTypeDef(name, reqInfo)
                ->getVisibility()
                .isAccessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

DefinitionType *
QatModule::getTypeDef(const String               &name,
                      const utils::RequesterInfo &reqInfo) const {
  for (auto *tDef : typeDefs) {
    if (tDef->getName() == name) {
      return tDef;
    }
  }
  for (auto brought : broughtTypeDefs) {
    if (!brought.isNamed()) {
      auto *tDef = brought.get();
      if (tDef->getName() == name) {
        return tDef;
      }
    } else if (brought.getName() == name) {
      return brought.get();
    }
  }
  for (auto brought : broughtModules) {
    if (!brought.isNamed()) {
      auto *bMod = brought.get();
      if (!bMod->shouldPrefixName()) {
        if (bMod->hasTypeDef(name) || bMod->hasBroughtTypeDef(name) ||
            bMod->hasAccessibleTypeDefInImports(name, reqInfo).first) {
          if (bMod->getTypeDef(name, reqInfo)
                  ->getVisibility()
                  .isAccessible(reqInfo)) {
            return bMod->getTypeDef(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GLOBAL ENTITY

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

QatModule *QatModule::getClosestParentBox() {
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

QatModule *QatModule::getClosestParentLib() {
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

String QatModule::getFilePath() const { return filePath; }

bool QatModule::areNodesEmitted() const { return isEmitted; }

void QatModule::emitNodes(IR::Context *ctx) const {
  if (!isEmitted) {
    SHOW("About to emit for module: " << getFullName())
    for (auto *node : nodes) {
      (void)node->emit(ctx);
    }
    SHOW("Emission for module complete")
    isEmitted = true;
    for (auto *sub : submodules) {
      SHOW("About to emit for submodule: " << sub->getFullName())
      sub->emitNodes(ctx);
      SHOW("Emission complete, linking LLVM modules")
      llvm::Linker::linkModules(
          *llvmModule, std::unique_ptr<llvm::Module>(sub->getLLVMModule()));
    }
    if (moduleType == ModuleType::file) {
      SHOW("Module is a file")
      auto *cfg = cli::Config::get();
      SHOW("Creating llvm output path")
      auto llPath = (cfg->hasOutputPath() ? cfg->getOutputPath() : basePath) /
                    ".llvm" /
                    filePath.lexically_relative(basePath)
                        .replace_filename(filePath.filename())
                        .replace_extension(".ll");
      std::error_code errorCode;
      SHOW("Creating all folders in llvm output path")
      fs::create_directory(llPath.parent_path(), errorCode);
      if (!errorCode) {
        errorCode.clear();
        llvm::raw_fd_ostream fStream(llPath.string(), errorCode);
        if (!errorCode) {
          SHOW("Printing LLVM module")
          llvmModule->print(fStream, nullptr);
          SHOW("LLVM module printed")
          ctx->llvmOutputPaths.push_back(llPath);
        } else {
          SHOW("Could not open file for writing")
        }
        fStream.flush();
      } else {
        SHOW("Error could not create directory")
      }
    }
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

llvm::Module *QatModule::getLLVMModule() const { return llvmModule; }

void QatModule::linkNative(NativeUnit nval) {
  switch (nval) {
  case NativeUnit::printf: {
    if (!llvmModule->getFunction("printf")) {
      llvm::Function::Create(
          llvm::FunctionType::get(
              llvm::Type::getInt32Ty(llvmModule->getContext()),
              {llvm::Type::getInt8Ty(llvmModule->getContext())->getPointerTo()},
              true),
          llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "printf",
          llvmModule);
    }
    break;
  }
  case NativeUnit::malloc: {
    break;
  }
  case NativeUnit::pthread: {
    break;
  }
  }
}

nuo::Json QatModule::toJson() const {
  // FIXME - Change
  return nuo::Json()._("name", name);
}

// NOLINTBEGIN(readability-magic-numbers)

bool QatModule::hasIntegerBitwidth(u64 bits) const {
  if (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 ||
      bits == 128) {
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
  if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64 &&
      bits != 128) {
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
  if (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 ||
      bits == 128) {
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
  if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64 &&
      bits != 128) {
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