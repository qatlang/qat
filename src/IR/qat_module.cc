#include "./qat_module.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../cli/logger.hpp"
#include "../show.hpp"
#include "../utils/run_command.hpp"
#include "../utils/utils.hpp"
#include "brought.hpp"
#include "function.hpp"
#include "global_entity.hpp"
#include "link_names.hpp"
#include "lld/Common/Driver.h"
#include "types/core_type.hpp"
#include "types/definition.hpp"
#include "types/opaque.hpp"
#include "types/qat_type.hpp"
#include "types/region.hpp"
#include "types/string_slice.hpp"
#include "types/void.hpp"
#include "value.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

#if OS_IS_WINDOWS
#if RUNTIME_IS_MINGW
#include <sdkddkver.h>
#include <windows.h>
#elif RUNTIME_IS_MSVC
#include <SDKDDKVer.h>
#include <Windows.h>
#endif
#endif

#define MACOS_DEFAULT_SDK_PATH "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"

namespace qat::ir {

Vec<Mod*>       Mod::allModules{};
Vec<fs::path>   Mod::usableNativeLibPaths{};
Maybe<String>   Mod::usableClangPath      = None;
Maybe<fs::path> Mod::windowsMSVCLibPath   = None;
Maybe<fs::path> Mod::windowsATLMFCLibPath = None;
Maybe<fs::path> Mod::windowsUCRTLibPath   = None;
Maybe<fs::path> Mod::windowsUMLibPath     = None;

void EntityState::do_next_phase(Mod* mod, Ctx* ctx) {
  SHOW("EntityState::do_next_phase " << (name.has_value() ? name.value().value : ""))
  auto nextPhaseVal = get_next_phase(currentPhase);
  if (nextPhaseVal.has_value()) {
    auto nextPhase = nextPhaseVal.value();
    if (astNode) {
      astNode->do_phase(nextPhase, mod, ctx);
    }
    currentPhase = nextPhase;
    if (phaseToPartial.has_value() && phaseToPartial.value() == nextPhase) {
      status = EntityStatus::partial;
    } else if (phaseToCompletion.has_value() && phaseToCompletion.value() == nextPhase) {
      status = EntityStatus::complete;
    } else if (phaseToChildrenPartial.has_value() && phaseToChildrenPartial.value() == nextPhase) {
      status = EntityStatus::childrenPartial;
    } else if ((nextPhase == maxPhase) && (nextPhase != ir::EmitPhase::phase_last)) {
      status = supportsChildren ? EntityStatus::childrenPartial : EntityStatus::complete;
    }
  }
}

void Mod::clear_all() {
  for (auto* mod : allModules) {
    delete mod;
  }
}

bool Mod::has_file_module(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_folder_module(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::folder) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

Mod* Mod::get_file_module(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return mod;
      }
    }
  }
  return nullptr;
}

Mod* Mod::get_folder_module(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::folder) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return mod;
      }
    }
  }
  return nullptr;
}

Mod::Mod(Identifier _name, fs::path _filepath, fs::path _basePath, ModuleType _type, const VisibilityInfo& _visibility,
         Ctx* ctx)
    : EntityOverview("module", Json(), _name.range), name(std::move(_name)), moduleType(_type),
      filePath(std::move(_filepath)), basePath(std::move(_basePath)), visibility(_visibility) {
  llvmModule = new llvm::Module(get_full_name(), ctx->llctx);
  llvmModule->setModuleIdentifier(get_full_name());
  llvmModule->setSourceFileName(filePath.string());
  llvmModule->setCodeModel(llvm::CodeModel::Small);
  llvmModule->setSDKVersion(cli::Config::get()->get_version_tuple());
  llvmModule->setTargetTriple(cli::Config::get()->get_target_triple());
  if (ctx->dataLayout) {
    llvmModule->setDataLayout(ctx->dataLayout.value());
  }
  allModules.push_back(this);
}

Mod::~Mod() {
  SHOW("Deleting module " << name.value << " in file " << filePath.string())
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
  for (auto* ent : entityEntries) {
    delete ent;
  }
}

void Mod::entity_name_check(Ctx* ctx, Identifier name, ir::EntityType entTy) {
  for (auto ent : entityEntries) {
    if (ent->name.has_value() && (ent->name->value == name.value)) {
      ctx->Error("Found " + entity_type_to_string(ent->type) + " named " + ctx->color(name.value) +
                     " in the parent module. So cannot define a " + entity_type_to_string(entTy) + " named " +
                     ctx->color(name.value) + ".",
                 name.range,
                 ent->name.has_value()
                     ? Maybe<Pair<String, FileRange>>(Pair<String, FileRange>{
                           "The existing " + entity_type_to_string(ent->type) + " can be found here", ent->name->range})
                     : None);
    }
  }
}

Mod* Mod::create(const Identifier& name, const fs::path& filepath, const fs::path& basePath, ModuleType type,
                 const VisibilityInfo& visib_info, Ctx* ctx) {
  return new Mod(name, filepath, basePath, type, visib_info, ctx);
}

Vec<Function*> Mod::collect_mod_initialisers() {
  Vec<Function*> res;
  for (auto* mod : allModules) {
    if (mod->nonConstantGlobals > 0) {
      res.push_back(mod->moduleInitialiser);
    }
  }
  return res;
}

ModuleType Mod::get_mod_type() const { return moduleType; }

const VisibilityInfo& Mod::get_visibility() const { return visibility; }

Mod* Mod::get_active() { // NOLINT(misc-no-recursion)
  if (active) {
    return active->get_active();
  } else {
    return this;
  }
}

Mod* Mod::get_parent_file() { // NOLINT(misc-no-recursion)
  if ((moduleType == ModuleType::file) || rootLib) {
    return this;
  } else {
    if (parent) {
      return parent->get_parent_file();
    }
  }
  return this;
}

bool Mod::has_meta_info_key(String key) const { return metaInfo.has_value() && metaInfo.value().has_key(key); }

bool Mod::has_meta_info_key_in_parent(String key) const {
  if (metaInfo.has_value() && metaInfo.value().has_key(key)) {
    return true;
  }
  if (parent) {
    return parent->has_meta_info_key_in_parent(key);
  }
  return false;
}

Maybe<ir::PrerunValue*> Mod::get_meta_info_for_key(String key) const {
  if (metaInfo.has_value() && metaInfo.value().has_key(key)) {
    return metaInfo.value().get_value_for(key);
  }
  return None;
}

Maybe<ir::PrerunValue*> Mod::get_meta_info_from_parent(String key) const {
  if (metaInfo.has_value() && metaInfo.value().has_key(key)) {
    return metaInfo.value().get_value_for(key);
  }
  if (parent) {
    return parent->get_meta_info_from_parent(key);
  }
  return None;
}

Maybe<String> Mod::get_relevant_foreign_id() const {
  if (moduleForeignID.has_value()) {
    return moduleForeignID;
  } else {
    if (metaInfo.has_value()) {
      if (metaInfo->has_key("foreign")) {
        moduleForeignID = metaInfo->get_foreign_id();
        return moduleForeignID;
      }
    }
    auto keyVal = get_meta_info_from_parent("foreign");
    if (keyVal.has_value()) {
      auto value = keyVal.value();
      if (value->get_ir_type()->is_string_slice()) {
        moduleForeignID = ir::StringSliceType::value_to_string(value);
        return moduleForeignID;
      } else {
        return None;
      }
    }
    return None;
  }
}

bool Mod::has_nth_parent(u32 n) const {
  if (n == 1) {
    return (parent != nullptr);
  } else if (n > 1) {
    return has_nth_parent(n - 1);
  } else {
    return true;
  }
}

Mod* Mod::get_nth_parent(u32 n) { // NOLINT(misc-no-recursion)
  if (has_nth_parent(n)) {
    if (n == 1) {
      return parent;
    } else if (n > 1) {
      return get_nth_parent(n - 1);
    } else {
      return this;
    }
  } else {
    return nullptr;
  }
}

void Mod::addMember(Mod* mod) {
  mod->parent = this;
  submodules.push_back(mod);
}

Function* Mod::create_function(const Identifier& name, bool isInline, Type* returnType, Vec<Argument> args,
                               bool isVariadic, const FileRange& fileRange, const VisibilityInfo& visibility,
                               Maybe<llvm::GlobalValue::LinkageTypes> linkage, Ctx* ctx) {
  SHOW("Creating IR function")
  auto nmUnits = get_link_names();
  nmUnits.addUnit(LinkNameUnit(name.value, LinkUnitType::function, {}), None);
  auto* fun = Function::Create(this, name, nmUnits, {/* Generics */}, isInline, ir::ReturnType::get(returnType),
                               std::move(args), isVariadic, fileRange, visibility, ctx, linkage);
  SHOW("Created function")
  functions.push_back(fun);
  return fun;
}

bool Mod::triple_is_equivalent(const llvm::Triple& first, const llvm::Triple& second) {
  if (first == second) {
    return true;
  } else {
    return (first.getOS() == second.getOS()) && (first.getArch() == second.getArch()) &&
           (first.getEnvironment() == second.getEnvironment()) && (first.getObjectFormat() == second.getObjectFormat());
  }
}

Mod* Mod::create_submodule(Mod* parent, fs::path filepath, fs::path basePath, Identifier sname, ModuleType type,
                           const VisibilityInfo& visibilityInfo, Ctx* ctx) {
  SHOW("Creating submodule: " << sname.value)
  auto* sub = new Mod(std::move(sname), std::move(filepath), std::move(basePath), type, visibilityInfo, ctx);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
    SHOW("Created submodule")
  }
  return sub;
}

Mod* Mod::create_file_mod(Mod* parent, fs::path filepath, fs::path basePath, Identifier fname, Vec<ast::Node*> nodes,
                          VisibilityInfo visibilityInfo, Ctx* ctx) {
  auto* sub =
      new Mod(std::move(fname), std::move(filepath), std::move(basePath), ModuleType::file, visibilityInfo, ctx);
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  sub->nodes = std::move(nodes);
  return sub;
}

Mod* Mod::create_root_lib(Mod* parent, fs::path filepath, fs::path basePath, Identifier fname, Vec<ast::Node*> nodes,
                          const VisibilityInfo& visibilityInfo, Ctx* ctx) {
  auto* sub = new Mod(std::move(fname), std::move(filepath), std::move(basePath), ModuleType::lib, visibilityInfo, ctx);
  sub->rootLib = true;
  if (parent) {
    sub->parent = parent;
    parent->submodules.push_back(sub);
  }
  sub->nodes = std::move(nodes);
  return sub;
}

void Mod::add_fs_bring_mention(Mod* otherMod, const FileRange& fileRange) {
  fsBroughtMentions.push_back(Pair<Mod*, FileRange>(otherMod, fileRange));
}

Vec<Pair<Mod*, FileRange>> const& Mod::get_fs_bring_mentions() const { return fsBroughtMentions; }

void Mod::update_overview() {
  String moduleTyStr;
  switch (moduleType) {
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
  ovInfo._("moduleID", get_id())
      ._("fullName", get_full_name())
      ._("isFilesystemLib", rootLib)
      ._("moduleType", moduleTyStr)
      ._("visibility", visibility)
      ._("hasModuleInitialiser", ((moduleInitialiser != nullptr) && (nonConstantGlobals > 0)))
      ._("integerBitwidths", integerBitsVal)
      ._("unsignedBitwidths", unsignedBitsVal)
      ._("filesystemBroughtMentions", fsBroughtMentionsJson);
}

void Mod::output_all_overview(Vec<JsonValue>& modulesJson, Vec<JsonValue>& functionsJson,
                              Vec<JsonValue>& genericFunctionsJson, Vec<JsonValue>& genericCoreTypesJson,
                              Vec<JsonValue>& coreTypesJson, Vec<JsonValue>& mixTypesJson, Vec<JsonValue>& regionJson,
                              Vec<JsonValue>& choiceJson, Vec<JsonValue>& defsJson) {
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
    for (auto* tDef : genericTypeDefinitions) {
      defsJson.push_back(tDef->overviewToJson());
    }
    for (auto* sub : submodules) {
      sub->output_all_overview(modulesJson, functionsJson, genericFunctionsJson, genericCoreTypesJson, coreTypesJson,
                               mixTypesJson, regionJson, choiceJson, defsJson);
    }
  }
}

String Mod::get_name() const { return name.value; }

void Mod::add_dependency(Mod* dep) { dependencies.insert(dep); }

Identifier Mod::get_identifier() const { return name; }

String Mod::get_full_name() const {
  if (parent) {
    if (parent->should_be_named()) {
      if (should_be_named()) {
        return parent->get_full_name() + ":" + name.value;
      } else {
        return parent->get_full_name();
      }
    } else {
      if (should_be_named()) {
        return name.value;
      } else {
        return "";
      }
    }
  } else {
    return should_be_named() ? name.value : "";
  }
}

String Mod::get_writable_name() const {
  String result;
  if (parent) {
    result = parent->get_writable_name();
    if (!result.empty()) {
      result += "_";
    }
  }
  switch (moduleType) {
    case ModuleType::lib: {
      result += ((rootLib ? "file-" : "lib-") + name.value);
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

String Mod::get_fullname_with_child(const String& child) const {
  if (should_be_named()) {
    return get_full_name() + ":" + child;
  } else {
    return parent ? parent->get_fullname_with_child(child) : child;
  }
}

bool Mod::is_in_foreign_mod_of_type(String id) const {
  if (get_relevant_foreign_id().has_value() && get_relevant_foreign_id().value() == id) {
    return true;
  }
  return false;
}

LinkNames Mod::get_link_names() const {
  if (parent) {
    auto parentVal = parent->get_link_names();
    if (should_be_named()) {
      parentVal.addUnit(LinkNameUnit(name.value, LinkUnitType::lib, {}), get_relevant_foreign_id());
    }
    return parentVal;
  } else {
    if (should_be_named()) {
      return LinkNames({LinkNameUnit(name.value, LinkUnitType::lib, {})}, get_relevant_foreign_id(), nullptr);
    } else {
      return LinkNames({}, get_relevant_foreign_id(), nullptr);
    }
  }
}

bool Mod::should_be_named() const { return moduleType == ModuleType::lib; }

Function* Mod::get_mod_initialiser(Ctx* ctx) {
  if (!moduleInitialiser) {
    moduleInitialiser = ir::Function::Create(
        this, Identifier("module'initialiser'" + utils::unique_id(), {filePath}), None, {/* Generics */}, false,
        ir::ReturnType::get(ir::VoidType::get(ctx->llctx)), {}, false, name.range, VisibilityInfo::pub(), ctx);
    auto* entry = new ir::Block(moduleInitialiser, nullptr);
    entry->set_active(ctx->builder);
  }
  return moduleInitialiser;
}

void Mod::add_non_const_global_counter() { nonConstantGlobals++; }

bool Mod::should_call_initialiser() const { return nonConstantGlobals != 0; }

void Mod::addNamedSubmodule(const Identifier& sname, const String& filename, ModuleType type,
                            const VisibilityInfo& visib_info, Ctx* ctx) {
  SHOW("Creating submodule: " << sname.value)
  active = create_submodule(this, filename, basePath.string(), sname, type, visib_info, ctx);
}

void Mod::closeSubmodule() { active = nullptr; }

bool Mod::has_lib(const String& name, AccessInfo reqInfo) const {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->get_name() == name) &&
        (sub->get_visibility().is_accessible(reqInfo))) {
      return true;
    } else if (!sub->should_be_named()) {
      if (sub->has_lib(name, reqInfo) || sub->has_brought_lib(name, reqInfo) ||
          sub->has_lib_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_lib(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      auto result = false;
      if (brought.is_named()) {
        result = (brought.name.value().value == name) && brought.visibility.is_accessible(reqInfo) &&
                 brought.entity->get_visibility().is_accessible(reqInfo);
      } else {
        result = (brought.entity->get_name() == name) && brought.visibility.is_accessible(reqInfo) &&
                 brought.entity->get_visibility().is_accessible(reqInfo);
      }
      if (result) {
        return true;
      }
    }
  }
  return false;
}

Pair<bool, String> Mod::has_lib_in_imports( // NOLINT(misc-no-recursion)
    const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_lib(name, reqInfo) || bMod->has_brought_lib(name, reqInfo) ||
            bMod->has_lib_in_imports(name, reqInfo).first) {
          if (bMod->get_lib(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

Mod* Mod::get_lib(const String& name, const AccessInfo& reqInfo) {
  for (auto* sub : submodules) {
    if ((sub->moduleType == ModuleType::lib) && (sub->get_name() == name) &&
        (sub->get_visibility().is_accessible(reqInfo))) {
      return sub;
    } else {
      if (!sub->should_be_named()) {
        if (sub->has_lib(name, reqInfo) || sub->has_brought_lib(name, reqInfo) ||
            sub->has_lib_in_imports(name, reqInfo).first) {
          return sub;
        }
      }
    }
  }
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (bMod->moduleType == ModuleType::lib) {
      auto result = false;
      if (brought.is_named()) {
        result = (brought.name.value().value == name) && brought.visibility.is_accessible(reqInfo) &&
                 brought.entity->get_visibility().is_accessible(reqInfo);
      } else {
        result = (brought.entity->get_name() == name) && brought.visibility.is_accessible(reqInfo) &&
                 brought.entity->get_visibility().is_accessible(reqInfo);
      }
      if (result) {
        return bMod;
      }
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_lib(name, reqInfo) || bMod->has_brought_lib(name, reqInfo) ||
            bMod->has_lib_in_imports(name, reqInfo).first) {
          if (bMod->get_lib(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_lib(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

void Mod::open_lib_for_creation(const Identifier& name, const String& filename, const VisibilityInfo& visib_info,
                                Ctx* ctx) {
  if (!has_lib(name.value, AccessInfo::GetPrivileged())) {
    addNamedSubmodule(name, filename, ModuleType::lib, visib_info, ctx);
  }
}

void Mod::close_lib_after_creation() { closeSubmodule(); }

bool Mod::has_brought_mod(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto result = false;
    if (brought.is_named()) {
      result = (brought.name.value().value == name) && brought.visibility.is_accessible(reqInfo) &&
               brought.entity->get_visibility().is_accessible(reqInfo);
    } else {
      result = (brought.entity->get_name() == name) && brought.visibility.is_accessible(reqInfo) &&
               brought.entity->get_visibility().is_accessible(reqInfo);
    }
    if (result) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_brought_mod_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_brought_mod(name, reqInfo) || bMod->has_brought_mod_in_imports(name, reqInfo).first) {
          if (bMod->get_brought_mod(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            SHOW("Found module in imports")
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

Mod* Mod::get_brought_mod(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    auto* bMod = brought.get();
    if (!brought.is_named()) {
      if (bMod->should_be_named() && (bMod->get_name() == name) && brought.get_visibility().is_accessible(reqInfo)) {
        return bMod;
      } else if (!bMod->should_be_named()) {
        if (bMod->has_brought_mod(name, reqInfo) || bMod->has_brought_mod_in_imports(name, reqInfo).first) {
          auto resMod = bMod->get_brought_mod(name, reqInfo);
          if (resMod->get_visibility().is_accessible(reqInfo)) {
            return resMod;
          }
        }
      }
    } else if ((brought.get_name().value == name) && brought.get_visibility().is_accessible(reqInfo)) {
      return bMod;
    }
  }
  return nullptr;
}

void Mod::bring_module(Mod* other, const VisibilityInfo& _visibility, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtModules.push_back(Brought<Mod>(bName.value(), other, _visibility));
  } else {
    broughtModules.push_back(Brought<Mod>(other, _visibility));
  }
}

void Mod::bring_struct_type(StructType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtCoreTypes.push_back(Brought<StructType>(bName.value(), cTy, visib));
  } else {
    broughtCoreTypes.push_back(Brought<StructType>(cTy, visib));
  }
}

void Mod::bring_opaque_type(OpaqueType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtOpaqueTypes.push_back(Brought<OpaqueType>(bName.value(), cTy, visib));
  } else {
    broughtOpaqueTypes.push_back(Brought<OpaqueType>(cTy, visib));
  }
}

void Mod::bring_generic_struct_type(GenericStructType* gCTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericCoreTypes.push_back(Brought<GenericStructType>(bName.value(), gCTy, visib));
  } else {
    broughtGenericCoreTypes.push_back(Brought<GenericStructType>(gCTy, visib));
  }
}

void Mod::bring_generic_type_definition(GenericDefinitionType* gTyDef, const VisibilityInfo& visib,
                                        Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericTypeDefinitions.push_back(Brought<GenericDefinitionType>(bName.value(), gTyDef, visib));
  } else {
    broughtGenericTypeDefinitions.push_back(Brought<GenericDefinitionType>(gTyDef, visib));
  }
}

void Mod::bring_mix_type(MixType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtMixTypes.push_back(Brought<MixType>(bName.value(), mTy, visib));
  } else {
    broughtMixTypes.push_back(Brought<MixType>(mTy, visib));
  }
}

void Mod::bring_choice_type(ChoiceType* chTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtChoiceTypes.push_back(Brought<ChoiceType>(bName.value(), chTy, visib));
  } else {
    broughtChoiceTypes.push_back(Brought<ChoiceType>(chTy, visib));
  }
}

void Mod::bring_type_definition(DefinitionType* dTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtTypeDefs.push_back(Brought<DefinitionType>(bName.value(), dTy, visib));
  } else {
    broughtTypeDefs.push_back(Brought<DefinitionType>(dTy, visib));
  }
}

void Mod::bring_function(Function* fn, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtFunctions.push_back(Brought<Function>(bName.value(), fn, visib));
  } else {
    broughtFunctions.push_back(Brought<Function>(fn, visib));
  }
}

void Mod::bring_generic_function(GenericFunction* gFn, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericFunctions.push_back(Brought<GenericFunction>(bName.value(), gFn, visib));
  } else {
    broughtGenericFunctions.push_back(Brought<GenericFunction>(gFn, visib));
  }
}

void Mod::bring_region(Region* reg, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtRegions.push_back(Brought<Region>(bName.value(), reg, visib));
  } else {
    broughtRegions.push_back(Brought<Region>(reg, visib));
  }
}

void Mod::bring_global(GlobalEntity* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGlobalEntities.push_back(Brought<GlobalEntity>(bName.value(), gEnt, visib));
  } else {
    broughtGlobalEntities.push_back(Brought<GlobalEntity>(gEnt, visib));
  }
}

void Mod::bring_prerun_global(PrerunGlobal* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtPrerunGlobals.push_back(Brought<PrerunGlobal>(bName.value(), gEnt, visib));
  } else {
    broughtPrerunGlobals.push_back(Brought<PrerunGlobal>(gEnt, visib));
  }
}

bool Mod::has_function(const String& name, AccessInfo reqInfo) const {
  SHOW("Function to be checked: " << name)
  SHOW("Module name " << get_name())
  SHOW("Module pointer: " << this)
  SHOW("Function count: " << functions.size())
  for (auto* function : functions) {
    SHOW("Function in module: " << function)
    if ((function->get_name().value == name) && function->get_visibility().is_accessible(reqInfo)) {
      SHOW("Found function")
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_function(name, reqInfo) || sub->has_brought_function(name, reqInfo) ||
          sub->has_function_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool Mod::has_brought_function(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_function_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        SHOW("Checking brought function " << name << " in brought module " << bMod->get_name() << " from file "
                                          << bMod->get_file_path())
        if (bMod->has_function(name, reqInfo) || bMod->has_brought_function(name, reqInfo) ||
            bMod->has_function_in_imports(name, reqInfo).first) {
          if (bMod->get_function(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

Function* Mod::get_function(const String& name, const AccessInfo& reqInfo) {
  for (auto* function : functions) {
    if ((function->get_name().value == name) && function->get_visibility().is_accessible(reqInfo)) {
      return function;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_function(name, reqInfo) || sub->has_brought_function(name, reqInfo) ||
          sub->has_function_in_imports(name, reqInfo).first) {
        return sub->get_function(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_function(name, reqInfo) || bMod->has_brought_function(name, reqInfo) ||
            bMod->has_function_in_imports(name, reqInfo).first) {
          if (bMod->get_function(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_function(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC FUNCTION

bool Mod::has_generic_function(const String& name, AccessInfo reqInfo) const {
  for (auto* function : genericFunctions) {
    if ((function->get_name().value == name) && function->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_function(name, reqInfo) || sub->has_brought_generic_function(name, reqInfo) ||
          sub->has_generic_function_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  SHOW("No functions named " + name + " found")
  return false;
}

bool Mod::has_brought_generic_function(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_generic_function_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_function(name, reqInfo) || bMod->has_brought_generic_function(name, reqInfo) ||
            bMod->has_generic_function_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_function(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericFunction* Mod::get_generic_function(const String& name, const AccessInfo& reqInfo) {
  for (auto* function : genericFunctions) {
    if ((function->get_name().value == name) && function->get_visibility().is_accessible(reqInfo)) {
      return function;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_function(name, reqInfo) || sub->has_brought_generic_function(name, reqInfo) ||
          sub->has_generic_function_in_imports(name, reqInfo).first) {
        return sub->get_generic_function(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericFunctions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_function(name, reqInfo) || bMod->has_brought_generic_function(name, reqInfo) ||
            bMod->has_generic_function_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_function(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_generic_function(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// REGION

bool Mod::has_region(const String& name, AccessInfo reqInfo) const {
  for (auto* reg : regions) {
    if ((reg->get_name().value == name) && reg->is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_region(name, reqInfo) || sub->has_brought_region(name, reqInfo) ||
          sub->has_region_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_region(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtRegions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_region_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() && (bMod->has_region(name, reqInfo) || bMod->has_brought_region(name, reqInfo) ||
                                       bMod->has_region_in_imports(name, reqInfo).first)) {
        if (bMod->get_region(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

Region* Mod::get_region(const String& name, const AccessInfo& reqInfo) const {
  for (auto* reg : regions) {
    if ((reg->get_name().value == name) && reg->is_accessible(reqInfo)) {
      return reg;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_region(name, reqInfo) || sub->has_brought_region(name, reqInfo) ||
          sub->has_region_in_imports(name, reqInfo).first) {
        return sub->get_region(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtRegions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_region(name, reqInfo) || bMod->has_brought_region(name, reqInfo) ||
            bMod->has_region_in_imports(name, reqInfo).first) {
          if (bMod->get_region(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_region(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// OPAQUE TYPE

bool Mod::has_opaque_type(const String& name, AccessInfo reqInfo) const {
  //   SHOW("Module " << this->name.value << " Opaque count: " << opaqueTypes.size())
  SHOW("Checking opaques in mod")
  SHOW("Opaque type count " << opaqueTypes.size())
  for (auto* typ : opaqueTypes) {
    SHOW("Opaque type name " << typ->get_name().value)
    if ((typ->get_name().value == name) && typ->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  SHOW("Checking submods")
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_opaque_type(name, reqInfo) || sub->has_brought_opaque_type(name, reqInfo) ||
          sub->has_opaque_type_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_opaque_type(const String& name, Maybe<AccessInfo> reqInfo) const {
  SHOW("Brought opaque type count " << broughtOpaqueTypes.size())
  for (const auto& brought : broughtOpaqueTypes) {
    SHOW("Brought entity " << (brought.name.has_value() ? ("Has Name " + brought.name.value().value)
                                                        : ("No Name" + brought.get()->get_name().value)))
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_opaque_type_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() &&
          (bMod->has_opaque_type(name, reqInfo) || bMod->has_brought_opaque_type(name, reqInfo) ||
           bMod->has_opaque_type_in_imports(name, reqInfo).first)) {
        if (bMod->get_opaque_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

OpaqueType* Mod::get_opaque_type(const String& name, const AccessInfo& reqInfo) const {
  for (auto* opaqueType : opaqueTypes) {
    if ((opaqueType->get_name().value == name) && opaqueType->get_visibility().is_accessible(reqInfo)) {
      return opaqueType;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_opaque_type(name, reqInfo) || sub->has_brought_opaque_type(name, reqInfo) ||
          sub->has_opaque_type_in_imports(name, reqInfo).first) {
        return sub->get_opaque_type(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtOpaqueTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_opaque_type(name, reqInfo) || bMod->has_brought_opaque_type(name, reqInfo) ||
            bMod->has_opaque_type_in_imports(name, reqInfo).first) {
          if (bMod->get_opaque_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_opaque_type(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// CORE TYPE

bool Mod::has_struct_type(const String& name, AccessInfo reqInfo) const {
  for (auto* typ : coreTypes) {
    if ((typ->get_name().value == name) && typ->is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_struct_type(name, reqInfo) || sub->has_brought_struct_type(name, reqInfo) ||
          sub->has_struct_type_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const {
  SHOW("")
  for (const auto& brought : broughtCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      SHOW("Brought module: " << bMod->get_id() << " name: " << bMod->get_full_name())
      if (!bMod->should_be_named() &&
          (bMod->has_struct_type(name, reqInfo) || bMod->has_brought_struct_type(name, reqInfo) ||
           bMod->has_struct_type_in_imports(name, reqInfo).first)) {
        if (bMod->get_struct_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

StructType* Mod::get_struct_type(const String& name, const AccessInfo& reqInfo) const {
  for (auto* coreType : coreTypes) {
    if ((coreType->get_name().value == name) && coreType->is_accessible(reqInfo)) {
      return coreType;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_struct_type(name, reqInfo) || sub->has_brought_struct_type(name, reqInfo) ||
          sub->has_struct_type_in_imports(name, reqInfo).first) {
        return sub->get_struct_type(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_struct_type(name, reqInfo) || bMod->has_brought_struct_type(name, reqInfo) ||
            bMod->has_struct_type_in_imports(name, reqInfo).first) {
          if (bMod->get_struct_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_struct_type(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// MIX TYPE

bool Mod::has_mix_type(const String& name, AccessInfo reqInfo) const {
  SHOW("MixType count: " << mixTypes.size())
  for (auto* typ : mixTypes) {
    if ((typ->get_name().value == name) && typ->is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_mix_type(name, reqInfo) || sub->has_brought_mix_type(name, reqInfo) ||
          sub->has_mix_type_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_mix_type(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtMixTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_mix_type_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() && (bMod->has_mix_type(name, reqInfo) || bMod->has_brought_mix_type(name, reqInfo) ||
                                       bMod->has_mix_type_in_imports(name, reqInfo).first)) {
        if (bMod->get_mix_type(name, reqInfo)->is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

MixType* Mod::get_mix_type(const String& name, const AccessInfo& reqInfo) const {
  for (auto* mixTy : mixTypes) {
    if ((mixTy->get_name().value == name) && mixTy->is_accessible(reqInfo)) {
      return mixTy;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_mix_type(name, reqInfo) || sub->has_brought_mix_type(name, reqInfo) ||
          sub->has_mix_type_in_imports(name, reqInfo).first) {
        return sub->get_mix_type(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtMixTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_mix_type(name, reqInfo) || bMod->has_brought_mix_type(name, reqInfo) ||
            bMod->has_mix_type_in_imports(name, reqInfo).first) {
          if (bMod->get_mix_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_mix_type(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// CHOICE TYPE

bool Mod::has_choice_type(const String& name, AccessInfo reqInfo) const {
  SHOW("ChoiceType count: " << mixTypes.size())
  for (auto* typ : choiceTypes) {
    if ((typ->get_name().value == name) && typ->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_choice_type(name, reqInfo) || sub->has_brought_choice_type(name, reqInfo) ||
          sub->has_choice_type_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_choice_type(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtChoiceTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_choice_type_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() &&
          (bMod->has_choice_type(name, reqInfo) || bMod->has_brought_choice_type(name, reqInfo) ||
           bMod->has_choice_type_in_imports(name, reqInfo).first)) {
        if (bMod->get_choice_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

ChoiceType* Mod::get_choice_type(const String& name, const AccessInfo& reqInfo) const {
  for (auto* choiceTy : choiceTypes) {
    if ((choiceTy->get_name().value == name) && choiceTy->get_visibility().is_accessible(reqInfo)) {
      return choiceTy;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_choice_type(name, reqInfo) || sub->has_brought_choice_type(name, reqInfo) ||
          sub->has_choice_type_in_imports(name, reqInfo).first) {
        return sub->get_choice_type(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtChoiceTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_choice_type(name, reqInfo) || bMod->has_brought_choice_type(name, reqInfo) ||
            bMod->has_choice_type_in_imports(name, reqInfo).first) {
          if (bMod->get_choice_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_choice_type(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC CORE TYPE

bool Mod::has_generic_struct_type(const String& name, AccessInfo reqInfo) const {
  for (auto* tempCTy : genericCoreTypes) {
    SHOW("Generic core type: " << tempCTy->get_name().value)
    if ((tempCTy->get_name().value == name) && tempCTy->get_visibility().is_accessible(reqInfo)) {
      SHOW("Found generic core type")
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_struct_type(name, reqInfo) || sub->has_brought_generic_struct_type(name, reqInfo) ||
          sub->has_generic_struct_type_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  SHOW("No generic core types named " + name + " found")
  return false;
}

bool Mod::has_brought_generic_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_generic_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_struct_type(name, reqInfo) || bMod->has_brought_generic_struct_type(name, reqInfo) ||
            bMod->has_generic_struct_type_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_struct_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericStructType* Mod::get_generic_struct_type(const String& name, const AccessInfo& reqInfo) {
  for (auto* tempCore : genericCoreTypes) {
    if ((tempCore->get_name().value == name) && tempCore->get_visibility().is_accessible(reqInfo)) {
      return tempCore;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_struct_type(name, reqInfo)) {
        return sub->get_generic_struct_type(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericCoreTypes) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_struct_type(name, reqInfo) || bMod->has_brought_generic_struct_type(name, reqInfo) ||
            bMod->has_generic_struct_type_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_struct_type(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_generic_struct_type(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// TYPEDEF

bool Mod::has_type_definition(const String& name, AccessInfo reqInfo) const {
  SHOW("Typedef count: " << typeDefs.size())
  for (auto* typ : typeDefs) {
    if ((typ->get_name().value == name) && typ->is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_type_definition(name, reqInfo) || sub->has_brought_type_definition(name, reqInfo) ||
          sub->has_type_definition_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_type_definition(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtTypeDefs) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_type_definition_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() &&
          (bMod->has_type_definition(name, reqInfo) || bMod->has_brought_type_definition(name, reqInfo) ||
           bMod->has_type_definition_in_imports(name, reqInfo).first)) {
        if (bMod->get_type_def(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

DefinitionType* Mod::get_type_def(const String& name, const AccessInfo& reqInfo) const {
  for (auto* tDef : typeDefs) {
    if ((tDef->get_name().value == name) && tDef->is_accessible(reqInfo)) {
      return tDef;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_type_definition(name, reqInfo) || sub->has_brought_type_definition(name, reqInfo) ||
          sub->has_type_definition_in_imports(name, reqInfo).first) {
        return sub->get_type_def(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtTypeDefs) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_type_definition(name, reqInfo) || bMod->has_brought_type_definition(name, reqInfo) ||
            bMod->has_type_definition_in_imports(name, reqInfo).first) {
          if (bMod->get_type_def(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_type_def(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// GENERIC TYPEDEF

bool Mod::has_generic_type_def(const String& name, AccessInfo reqInfo) const {
  for (auto* tempCTy : genericTypeDefinitions) {
    if ((tempCTy->get_name().value == name) && tempCTy->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_type_def(name, reqInfo) || sub->has_brought_generic_type_def(name, reqInfo) ||
          sub->has_generic_type_def_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_generic_type_def(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_generic_type_def_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_type_def(name, reqInfo) || bMod->has_brought_generic_type_def(name, reqInfo) ||
            bMod->has_generic_type_def_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_type_def(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return {true, bMod->filePath.string()};
          }
        }
      }
    }
  }
  return {false, ""};
}

GenericDefinitionType* Mod::get_generic_type_def(const String& name, const AccessInfo& reqInfo) {
  for (auto* tempDef : genericTypeDefinitions) {
    if ((tempDef->get_name().value == name) && tempDef->get_visibility().is_accessible(reqInfo)) {
      return tempDef;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_generic_type_def(name, reqInfo) || sub->has_brought_generic_type_def(name, reqInfo) ||
          sub->has_generic_type_def_in_imports(name, reqInfo).first) {
        return sub->get_generic_type_def(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGenericTypeDefinitions) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named()) {
        if (bMod->has_generic_type_def(name, reqInfo) || bMod->has_brought_generic_type_def(name, reqInfo) ||
            bMod->has_generic_type_def_in_imports(name, reqInfo).first) {
          if (bMod->get_generic_type_def(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
            return bMod->get_generic_type_def(name, reqInfo);
          }
        }
      }
    }
  }
  return nullptr;
}

// PRERUN GLOBAL

bool Mod::has_prerun_global(const String& name, AccessInfo reqInfo) const {
  for (auto* entity : prerunGlobals) {
    if ((entity->get_name().value == name) && entity->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_prerun_global(name, reqInfo) || sub->has_brought_prerun_global(name, reqInfo) ||
          sub->has_prerun_global_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_prerun_global(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtPrerunGlobals) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_prerun_global_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() &&
          (bMod->has_prerun_global(name, reqInfo) || bMod->has_brought_prerun_global(name, reqInfo) ||
           bMod->has_prerun_global_in_imports(name, reqInfo).first)) {
        if (bMod->get_prerun_global(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

PrerunGlobal* Mod::get_prerun_global(const String&     name, // NOLINT(misc-no-recursion)
                                     const AccessInfo& reqInfo) const {
  for (auto* ent : prerunGlobals) {
    if ((ent->get_name().value == name) && ent->get_visibility().is_accessible(reqInfo)) {
      return ent;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_prerun_global(name, reqInfo) || sub->has_brought_prerun_global(name, reqInfo) ||
          sub->has_prerun_global_in_imports(name, reqInfo).first) {
        return sub->get_prerun_global(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtPrerunGlobals) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() &&
          (bMod->has_prerun_global(name, reqInfo) || bMod->has_brought_prerun_global(name, reqInfo) ||
           bMod->has_prerun_global_in_imports(name, reqInfo).first)) {
        if (bMod->get_prerun_global(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return bMod->get_prerun_global(name, reqInfo);
        }
      }
    }
  }
  return nullptr;
}

// GLOBAL ENTITY

bool Mod::has_global(const String& name, AccessInfo reqInfo) const {
  for (auto* entity : globalEntities) {
    if ((entity->get_name().value == name) && entity->get_visibility().is_accessible(reqInfo)) {
      return true;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_global(name, reqInfo) || sub->has_brought_global(name, reqInfo) ||
          sub->has_global_in_imports(name, reqInfo).first) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::has_brought_global(const String& name, Maybe<AccessInfo> reqInfo) const {
  for (const auto& brought : broughtGlobalEntities) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return true;
    }
  }
  return false;
}

Pair<bool, String> Mod::has_global_in_imports(const String& name, const AccessInfo& reqInfo) const {
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() && (bMod->has_global(name, reqInfo) || bMod->has_brought_global(name, reqInfo) ||
                                       bMod->has_global_in_imports(name, reqInfo).first)) {
        if (bMod->get_global(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return {true, bMod->filePath.string()};
        }
      }
    }
  }
  return {false, ""};
}

GlobalEntity* Mod::get_global(const String&     name, // NOLINT(misc-no-recursion)
                              const AccessInfo& reqInfo) const {
  for (auto* ent : globalEntities) {
    if ((ent->get_name().value == name) && ent->get_visibility().is_accessible(reqInfo)) {
      return ent;
    }
  }
  for (auto sub : submodules) {
    if (!sub->should_be_named()) {
      if (sub->has_global(name, reqInfo) || sub->has_brought_global(name, reqInfo) ||
          sub->has_global_in_imports(name, reqInfo).first) {
        return sub->get_global(name, reqInfo);
      }
    }
  }
  for (const auto& brought : broughtGlobalEntities) {
    if (matchBroughtEntity(brought, name, reqInfo)) {
      return brought.get();
    }
  }
  for (const auto& brought : broughtModules) {
    if (!brought.is_named()) {
      auto* bMod = brought.get();
      if (!bMod->should_be_named() && (bMod->has_global(name, reqInfo) || bMod->has_brought_global(name, reqInfo) ||
                                       bMod->has_global_in_imports(name, reqInfo).first)) {
        if (bMod->get_global(name, reqInfo)->get_visibility().is_accessible(reqInfo)) {
          return bMod->get_global(name, reqInfo);
        }
      }
    }
  }
  return nullptr;
}

bool Mod::is_parent_mod_of(Mod* other) const {
  auto* othMod = other->parent;
  if (othMod) {
    while (othMod) {
      if (get_id() == othMod->get_id()) {
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

bool Mod::has_parent_lib() const {
  if (moduleType == ModuleType::lib) {
    return true;
  } else {
    if (parent) {
      return parent->has_parent_lib();
    } else {
      return false;
    }
  }
}

Mod* Mod::get_closest_parent_lib() {
  if (moduleType == ModuleType::lib) {
    return this;
  } else {
    if (parent) {
      return parent->get_closest_parent_lib();
    } else {
      return nullptr;
    }
  }
}

// bool Mod::areNodesEmitted() const { return isEmitted; }

void Mod::node_create_modules(Ctx* ctx) {
  if (!hasCreatedModules) {
    hasCreatedModules = true;
    SHOW("Creating modules via nodes \nname = " << name.value << "\n    Path = " << filePath.string())
    SHOW("    has_parent = " << (parent != nullptr))
    SHOW("Submodule count before creating modules via nodes: " << submodules.size())
    for (auto* node : nodes) {
      node->create_module(this, ctx);
    }
    SHOW("Submodule count after creating modules via nodes: " << submodules.size())
    for (auto* sub : submodules) {
      sub->node_create_modules(ctx);
    }
  }
}

void Mod::node_handle_fs_brings(Ctx* ctx) {
  if (!hasHandledFilesystemBrings) {
    hasHandledFilesystemBrings = true;
    for (auto* node : nodes) {
      node->handle_fs_brings(this, ctx);
    }
    for (auto* sub : submodules) {
      sub->node_handle_fs_brings(ctx);
    }
  }
}

void Mod::node_create_entities(Ctx* ctx) {
  if (!hasCreatedEntities) {
    hasCreatedEntities = true;
    for (auto* node : nodes) {
      if (node->is_entity()) {
        ((ast::IsEntity*)node)->create_entity(this, ctx);
      }
    }
    for (auto* sub : submodules) {
      sub->node_create_entities(ctx);
    }
  }
}

void Mod::node_update_dependencies(Ctx* ctx) {
  if (!hasUpdatedEntityDependencies) {
    hasUpdatedEntityDependencies = true;
    for (auto* node : nodes) {
      if (node->is_entity()) {
        ((ast::IsEntity*)node)->update_entity_dependencies(this, ctx);
      }
    }
    for (auto* sub : submodules) {
      sub->node_update_dependencies(ctx);
    }
  }
}

void Mod::setup_llvm_file(Ctx* ctx) {
  if (moduleInitialiser) {
    moduleInitialiser->get_block()->set_active(ctx->builder);
    ctx->builder.CreateRetVoid();
  }
  auto* cfg = cli::Config::get();
  SHOW("Creating llvm output path")
  auto fileName = get_writable_name() + ".ll";
  llPath        = (cfg->has_output_path() ? cfg->get_output_path() : basePath) / "llvm" /
           filePath.lexically_relative(basePath).replace_filename(fileName);
  std::error_code errorCode;
  if (fs::exists(llPath)) {
    fs::remove(llPath, errorCode);
    if (errorCode) {

      ctx->Error("Error while deleting existing file " + ctx->color(llPath.string()), None);
    }
  }
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
      ctx->Error("Error while writing the LLVM IR to the file " + ctx->color(llPath.string()), None);
    }
    fStream.flush();
    SHOW("Flushed llvm IR file stream")
  } else {
    ctx->Error("Error while creating parent directory for LLVM output file with path: " +
                   ctx->color(llPath.parent_path().string()) + " with error: " + errorCode.message(),
               None);
  }
  for (auto sub : submodules) {
    sub->setup_llvm_file(ctx);
  }
}

bool Mod::find_clang_path(Ctx* ctx) {
  if (usableClangPath.has_value()) {
    return true;
  }
  auto cfg = cli::Config::get();
  if (!cfg->has_clang_path()) {
    auto clangPath   = boost::process::search_path("clang");
    auto clang17Path = boost::process::search_path("clang-17");
    SHOW("clang path from boost is " << clangPath)
    SHOW("clang-17 path from boost is " << clang17Path)
    if (clangPath.empty() && clang17Path.empty()) {
      ctx->Error(
          ctx->color("clang") + " could not be found in PATH. Tried using " + ctx->color("clang-17") +
              " which is the compatible version of clang, also bundled with qat, but that executable also could not be found."
              " Please check your installation of qat. Make sure that clang is installed and is added to the system PATH variable."
              " If you want clang from a custom path to be used, provide path to clang using " +
              ctx->color("--clang=\"/path/of/clang/exe\""),
          None);
    } else if (clang17Path.empty() && !clangPath.empty()) {
      auto clangRes = run_command_get_stdout(clangPath.string(), {"--version"});
      if (clangRes.first == 0) {
        Maybe<String> versionString;
        if (clangRes.second.find("\n") != String::npos) {
          auto firstLine = clangRes.second.substr(0, clangRes.second.find("\n"));
          if (firstLine.find("version ") != String::npos) {
            versionString = firstLine.substr(firstLine.find("version ") + String::traits_type::length("version "));
          }
        } else if (clangRes.second.find("version ") != String::npos) {
          versionString =
              clangRes.second.substr(clangRes.second.find("version ") + String::traits_type::length("version "));
        }
        if (versionString.has_value() && versionString.value().find(' ') != String::npos) {
          versionString = versionString.value().substr(versionString.value().find(' '));
        }
        Maybe<int> majorVersion;
        if (versionString.has_value()) {
          if (versionString.value().find('.') != String::npos) {
            auto majorStr = versionString.value().substr(0, versionString.value().find('.'));
            if (utils::is_integer(majorStr)) {
              majorVersion = std::stoi(majorStr);
            }
          } else if (utils::is_integer(versionString.value())) {
            majorVersion = std::stoi(versionString.value());
          }
        }
        if (majorVersion.has_value()) {
          if (majorVersion.value() < 17) {
            ctx->Error(
                "qat requires a clang installation of version 17 or later. The clang compiler found at " +
                    clangPath.string() + " has a version of " + ctx->color(versionString.value()) +
                    ". Also tried using the " + ctx->color("clang-17") +
                    " binary bundled with qat, but the file could not be found. Please check your installation of qat",
                None);
          }
          usableClangPath = clangPath.string();
        } else {
          ctx->Error(
              "Could not determine the version of the clang compiler found at " + ctx->color(clangPath.string()) +
                  ". Also tried using the " + ctx->color("clang-17") +
                  " binary bundled with qat, but the file could not be found. Please check your installation of qat",
              None);
        }
      } else {
        ctx->Error(
            "Tried to run " + ctx->color("clang --version") +
                " to determine the version of the clang compiler. But the command failed with the status code " +
                ctx->color(std::to_string(clangRes.first)) + ". Also tried using the " + ctx->color("clang-17") +
                " binary bundled with qat, but the file could not be found. Please check your installation of qat",
            None);
      }
    } else if (!clang17Path.empty()) {
      usableClangPath = clang17Path.string();
    }
  } else {
    usableClangPath = cfg->get_clang_path();
  }
  return usableClangPath.has_value();
}

void Mod::compile_to_object(Ctx* ctx) {
  if (!isCompiledToObject) {
    auto& log = Logger::get();
    log->say("Compiling module `" + name.value + "` from file " + filePath.string());
    auto*       cfg = cli::Config::get();
    Vec<String> compileArgs;
    if (cfg->should_build_shared()) {
      compileArgs.push_back("-fPIC");
      compileArgs.push_back("-c");
      compileArgs.push_back("-mllvm");
      compileArgs.push_back("-enable-matrix");
    } else {
      compileArgs.push_back("-c");
      compileArgs.push_back("-mllvm");
      compileArgs.push_back("-enable-matrix");
    }
    if (linkPthread) {
      compileArgs.push_back("-pthread");
    }
    compileArgs.push_back("--target=" + ctx->clangTargetInfo->getTriple().getTriple());
    if (cfg->has_sysroot()) {
      compileArgs.push_back("--sysroot=" + cfg->get_sysroot());
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      compileArgs.push_back("-nostartfiles");
      compileArgs.push_back("-Wl,--no-entry");
      compileArgs.push_back("-Wl,--export-all");
    }
    for (auto* sub : submodules) {
      sub->compile_to_object(ctx);
    }
    objectFilePath = fs::absolute((cfg->has_output_path() ? cfg->get_output_path() : basePath) / "object" /
                                  filePath.lexically_relative(basePath).replace_filename(get_writable_name().append(
                                      ctx->clangTargetInfo->getTriple().isOSWindows()
                                          ? ".obj"
                                          : (ctx->clangTargetInfo->getTriple().isWasm() ? ".wasm" : ".o"))))
                         .lexically_normal();
    SHOW("Got object file path")
    if (!fs::exists(objectFilePath.value().parent_path())) {
      std::error_code errorCode;
      SHOW("Creating all folders in object file output path: " << objectFilePath.value())
      fs::create_directories(objectFilePath.value().parent_path(), errorCode);
      if (errorCode) {
        ctx->Error("Could not create parent directory for the object file. Parent directory path is: " +
                       ctx->color(objectFilePath.value().parent_path().string()) +
                       " with error: " + errorCode.message(),
                   None);
      }
    }
    compileArgs.push_back(llPath.string());
    compileArgs.push_back("-o");
    compileArgs.push_back(objectFilePath.value().string());
    auto clangFound = find_clang_path(ctx);
    SHOW("Clang is found: " << clangFound)
    // TODO - ?? Check if the provided clang compiler is above version 17
    auto cmdRes = run_command_get_stderr(usableClangPath.value(), compileArgs);
    if (cmdRes.first) {
      ctx->Error("Could not compile the LLVM file: " + ctx->color(filePath.string()) + ". The output is\n" +
                     cmdRes.second,
                 None);
    }
    isCompiledToObject = true;
  }
}

std::set<String> Mod::getAllObjectPaths() const {
  SHOW("GetAllObjectPaths for `" << name.value << "` in " << filePath.string())
  std::set<String> result;
  if (objectFilePath.has_value()) {
    SHOW("Object path has value")
    result.insert(objectFilePath.value().string());
  }
  auto moduleHandler = [&](Mod* modVal) {
    auto objList = modVal->getAllObjectPaths();
    for (auto& path : objList) {
      result.insert(path);
    }
  };
  for (auto dep : dependencies) {
    moduleHandler(dep);
  }
  for (auto sub : submodules) {
    moduleHandler(sub);
  }
  for (auto& bMod : broughtModules) {
    moduleHandler(bMod.get());
  }
  for (auto& bTy : broughtCoreTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericCoreTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtChoiceTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtOpaqueTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericOpaqueTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtMixTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtTypeDefs) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtFunctions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericFunctions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericTypeDefinitions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGlobalEntities) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtRegions) {
    moduleHandler(bTy.get()->getParent());
  }
  return result;
}

std::set<String> Mod::getAllLinkableLibs() const {
  SHOW("Linkable lib for " << name.value << " in " << filePath)
  std::set<String> result;
  if (!nativeLibsToLink.empty()) {
    for (auto& lib : nativeLibsToLink) {
      if (lib.type == LibToLinkType::namedLib) {
        result.insert("-l" + lib.name->value);
      } else if (lib.type == LibToLinkType::libPath) {
        result.insert("-l" + lib.path->first);
      }
    }
  }
  auto moduleHandler = [&](Mod* modVal) {
    auto libList = modVal->getAllLinkableLibs();
    for (auto& lib : libList) {
      result.insert(lib);
    }
    if (modVal->linkPthread) {
      linkPthread = true;
    }
  };
  for (auto dep : dependencies) {
    moduleHandler(dep);
  }
  for (auto sub : submodules) {
    moduleHandler(sub);
  }
  for (auto& bMod : broughtModules) {
    moduleHandler(bMod.get());
  }
  for (auto& bTy : broughtCoreTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericCoreTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtChoiceTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtOpaqueTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericOpaqueTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtMixTypes) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtTypeDefs) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtFunctions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericFunctions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGenericTypeDefinitions) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtGlobalEntities) {
    moduleHandler(bTy.get()->get_module());
  }
  for (auto& bTy : broughtRegions) {
    moduleHandler(bTy.get()->getParent());
  }
  return result;
}

void Mod::find_native_library_paths() {
  Vec<fs::path> unixPaths    = {"/lib", "/usr/lib", "/usr/local/lib"};
  auto          cfg          = cli::Config::get();
  const auto    hostTriple   = llvm::Triple(LLVM_HOST_TRIPLE);
  const auto    targetTriple = llvm::Triple(cfg->get_target_triple());
  if (hostTriple.getArch() == llvm::Triple::ArchType::x86_64) {
    unixPaths.push_back("/lib64");
    unixPaths.push_back("/usr/lib64");
    unixPaths.push_back("/usr/local/lib64");
  } else if (hostTriple.getArch() == llvm::Triple::ArchType::x86) {
    unixPaths.push_back("/lib32");
    unixPaths.push_back("/usr/lib32");
    unixPaths.push_back("/usr/local/lib32");
  }
  if (hostTriple.isOSLinux() || hostTriple.isOSDarwin()) {
    for (auto& item : unixPaths) {
      if (fs::exists(item)) {
        usableNativeLibPaths.push_back(item);
        auto targetStr =
            ((targetTriple.getArch() == llvm::Triple::ArchType::UnknownArch
                  ? ""
                  : (targetTriple.getArchName().str() + "-")) +
             (targetTriple.isOSUnknown()
                  ? ""
                  : (targetTriple.getOSName().str() +
                     (targetTriple.getEnvironment() == llvm::Triple::EnvironmentType::UnknownEnvironment ? "" : "-"))) +
             targetTriple.getEnvironmentName().str());
        if (fs::exists(item / targetStr)) {
          usableNativeLibPaths.push_back(item / targetStr);
        }
      }
    }
  } else if (hostTriple.isOSWindows()) {
#define WINDOWS_SYS32 "C:/Windows/System32"
    if (fs::exists(WINDOWS_SYS32)) {
      usableNativeLibPaths.push_back(WINDOWS_SYS32);
    }
  } else if (hostTriple.isOSCygMing()) {
#define MSYS_CLANG64 "C:/msys64/clang64/lib"
    if (fs::exists(MSYS_CLANG64)) {
      usableNativeLibPaths.push_back(MSYS_CLANG64);
    }
  }
  if (cfg->has_sysroot()) {
    if (fs::exists(fs::path(cfg->get_sysroot()) / "lib")) {
      usableNativeLibPaths.push_back(fs::path(cfg->get_sysroot()) / "lib");
    }
  }
}

Maybe<fs::path> Mod::find_static_library_path(String libName) const {
  SHOW("Finding static library path for " << libName)
  for (auto folder : usableNativeLibPaths) {
    if (fs::exists(fs::path(folder) / ("lib" + libName + ".a"))) {
      return fs::path(folder) / ("lib" + libName + ".a");
    }
  }
  return None;
}

#define LINK_LIB_KEY               "linkLibs"
#define LINK_LIBS_FROM_PATH_KEY    "linkLibsFromPath"
#define LINK_FILE_KEY              "linkFiles"
#define LINK_STATIC_AND_SHARED_KEY "linkStaticAndShared"

void Mod::handle_native_libs(Ctx* ctx) {
  SHOW("Module " << name.value << " has meta info: " << metaInfo.has_value())
  if (metaInfo.has_value()) {
    SHOW("MetaInfo has value")
    if (metaInfo.value().has_key(LINK_LIB_KEY)) {
      SHOW("Found linkLibs key")
      auto linkLibPre = metaInfo.value().get_value_for(LINK_LIB_KEY);
      if (linkLibPre->get_ir_type()->is_array() &&
          linkLibPre->get_ir_type()->as_array()->get_element_type()->is_string_slice()) {
        auto             dataArr = llvm::cast<llvm::ConstantArray>(linkLibPre->get_llvm_constant());
        std::set<String> libs;
        for (usize i = 0; i < linkLibPre->get_ir_type()->as_array()->get_length(); i++) {
          SHOW("Link lib array at: " << i)
          auto itemLib = ir::StringSliceType::value_to_string(
              ir::PrerunValue::get(dataArr->getAggregateElement(i), ir::StringSliceType::get(ctx)));
          SHOW("link item retrieved")
          if (!libs.contains(itemLib)) {
            nativeLibsToLink.push_back(
                LibToLink::fromName({itemLib, metaInfo.value().get_value_range_for(LINK_LIB_KEY)},
                                    metaInfo.value().get_value_range_for(LINK_LIB_KEY)));
            libs.insert(itemLib);
          }
        }
      } else {
        ctx->Error("The field " + ctx->color(LINK_LIB_KEY) +
                       " is expected to be an array of string slices. The provided value is of type " +
                       ctx->color(linkLibPre->get_ir_type()->to_string()),
                   metaInfo.value().get_value_range_for(LINK_LIB_KEY));
      }
    } else if (metaInfo.value().has_key(LINK_LIBS_FROM_PATH_KEY)) {
      ctx->Error("This is not supported for now", metaInfo->get_value_range_for(LINK_LIBS_FROM_PATH_KEY));
    } else if (metaInfo.value().has_key(LINK_FILE_KEY)) {
      auto linkFilePre = metaInfo.value().get_value_for(LINK_FILE_KEY);
      if (linkFilePre->get_ir_type()->is_array() &&
          linkFilePre->get_ir_type()->as_array()->get_element_type()->is_string_slice()) {
        auto             dataArr = llvm::cast<llvm::ConstantArray>(linkFilePre->get_llvm_constant());
        std::set<String> libs;
        for (usize i = 0; i < linkFilePre->get_ir_type()->as_array()->get_length(); i++) {
          auto itemLib = ir::StringSliceType::value_to_string(
              ir::PrerunValue::get(dataArr->getAggregateElement(i), ir::StringSliceType::get(ctx)));
          if (!libs.contains(itemLib)) {
            nativeLibsToLink.push_back(
                LibToLink::fromPath({itemLib, metaInfo.value().get_value_range_for(LINK_FILE_KEY)},
                                    metaInfo.value().get_value_range_for(LINK_FILE_KEY)));
            libs.insert(itemLib);
          }
        }
      } else {
        ctx->Error("The field " + ctx->color(LINK_FILE_KEY) +
                       " is expected to be an array of string slices. The provided value is of type " +
                       ctx->color(linkFilePre->get_ir_type()->to_string()),
                   metaInfo.value().get_value_range_for(LINK_FILE_KEY));
      }
    } else if (metaInfo.value().has_key(LINK_STATIC_AND_SHARED_KEY)) {
      ctx->Error("This is not supported for now", metaInfo->get_value_range_for(LINK_STATIC_AND_SHARED_KEY));
    }
  }
  for (auto* sub : submodules) {
    sub->handle_native_libs(ctx);
  }
}

bool Mod::find_windows_toolchain_libs(ir::Ctx* irCtx, bool findMSVCLibPath, bool findATLMFCLibPath,
                                      bool findUCRTLibPath, bool findUMLibPath) {
  auto cfg    = cli::Config::get();
  auto target = irCtx->clangTargetInfo->getTriple();
  if (cfg->has_toolchain_path()) {
    const auto windowsPath = cfg->get_toolchain_path() / "windows";
    if (fs::exists(windowsPath) && fs::is_directory(windowsPath)) {
      if (findMSVCLibPath || findATLMFCLibPath) {
        const auto msvcMainPath = windowsPath / "MSVC";
        if (fs::exists(msvcMainPath) && fs::is_directory(msvcMainPath)) {
          Maybe<int> version;
          for (auto it : fs::directory_iterator(msvcMainPath)) {
            if (it.is_directory()) {
              if (utils::is_integer(it.path().filename())) {
                if (version.has_value()) {
                  if (version.value() < std::stoi(it.path().filename())) {
                    version = std::stoi(it.path().filename());
                  }
                } else {
                  version = std::stoi(it.path().filename());
                }
              }
            }
          }
          if (version.has_value()) {
            auto            versionPath = msvcMainPath / std::to_string(version.value());
            Maybe<fs::path> candidateMSVCLibPath;
            Maybe<fs::path> candidateATLMFCLibPath;
            if (target.isX86() && target.isArch64Bit()) {
              candidateMSVCLibPath   = versionPath / "lib" / "x64";
              candidateATLMFCLibPath = versionPath / "atlmfc" / "lib" / "x64";
            } else if (target.isX86() && target.isArch32Bit()) {
              candidateMSVCLibPath   = versionPath / "lib" / "x86";
              candidateATLMFCLibPath = versionPath / "atlmfc" / "lib" / "x86";
            } else if (target.isAArch64()) {
              candidateMSVCLibPath = versionPath / "lib" / "arm64";
            } else if (target.isARM()) {
              candidateMSVCLibPath = versionPath / "lib" / "arm";
            } else if (target.isWindowsCoreCLREnvironment()) {
              if (target.isArch64Bit()) {
                candidateMSVCLibPath = versionPath / "lib" / "opencore" / "x64";
              } else {
                candidateMSVCLibPath = versionPath / "lib" / "opencore" / "x86";
              }
            }
            if (candidateMSVCLibPath.has_value() && fs::exists(candidateMSVCLibPath.value()) &&
                fs::is_directory(candidateMSVCLibPath.value())) {
              SHOW("Found MSVC Lib Path")
              windowsMSVCLibPath = candidateMSVCLibPath;
            }
            if (candidateATLMFCLibPath.has_value() && fs::exists(candidateATLMFCLibPath.value()) &&
                fs::is_directory(candidateATLMFCLibPath.value())) {
              SHOW("Found ATLMFC Lib Path")
              windowsATLMFCLibPath = candidateATLMFCLibPath;
            }
          } else {
            irCtx->Error("Could not find any versions of MSVC in the directory " + irCtx->color(msvcMainPath.string()),
                         None);
          }
        } else {
          irCtx->Error("Could not find the directory " + irCtx->color((windowsPath / "MSVC").string()) +
                           ", which is required for finding platform-specific libraries for the target " +
                           irCtx->color(irCtx->clangTargetInfo->getTriple().str()),
                       None);
        }
      } else if (findUCRTLibPath || findUMLibPath) {
        auto windowsKitPath = windowsPath / "SDK";
        if (fs::exists(windowsKitPath) && fs::is_directory(windowsKitPath)) {
          Maybe<fs::path> osVerPath;
          Maybe<float>    osVer;
          for (auto it : fs::directory_iterator(windowsKitPath)) {
            if (fs::is_directory(it)) {
              // FIXME - Rethink for future Windows SDK versions
              if (it.path().filename().string() == "10") {
                if (osVer.has_value() ? (osVer.value() < 10) : true) {
                  osVerPath = windowsKitPath / "10";
                  osVer     = 10;
                }
              } else if (it.path().filename().string() == "8.1") {
                if (osVer.has_value() ? (osVer.value() < 8.1) : true) {
                  osVerPath = windowsKitPath / "8.1";
                  osVer     = 8.1;
                }
              } else if (it.path().filename().string() == "7.1") {
                if (osVer.has_value() ? (osVer.value() < 7.1) : true) {
                  osVerPath = windowsKitPath / "7.1";
                  osVer     = 7.1;
                }
              }
            }
          }
          if (osVerPath.has_value() && fs::exists(osVerPath.value() / "Lib")) {
            SHOW("Found OS Ver Path: " << (osVerPath.value() / "Lib").string())
            std::array<u64, 4> verNums = {0, 0, 0, 0};
            fs::path           osVerSDKPath;
            for (auto it : fs::directory_iterator(osVerPath.value() / "Lib")) {
              if (fs::is_directory(it)) {
                SHOW("In OS Ver main path, iterating " << it.path().string())
                auto  osVersionStr = it.path().filename().string();
                usize dotCount     = 0;
                for (char ch : osVersionStr) {
                  if (ch == '.') {
                    dotCount++;
                  }
                }
                SHOW("Dot count is " << dotCount)
                if (dotCount == 3) {
                  auto               firstDot  = osVersionStr.find('.');
                  auto               secondDot = osVersionStr.find('.', firstDot + 1);
                  auto               thirdDot  = osVersionStr.find('.', secondDot + 1);
                  std::array<u64, 4> newVerNum = {std::stoul(osVersionStr.substr(0, firstDot - 1)),
                                                  std::stoul(osVersionStr.substr(firstDot + 1, secondDot - 1)),
                                                  std::stoul(osVersionStr.substr(secondDot + 1, thirdDot - 1)),
                                                  std::stoul(osVersionStr.substr(thirdDot + 1))};
                  for (usize i = 0; i < 4; i++) {
                    if (newVerNum[i] > verNums[i]) {
                      verNums      = newVerNum;
                      osVerSDKPath = it.path();
                      break;
                    } else if (newVerNum[i] == verNums[i]) {
                      continue;
                    } else {
                      break;
                    }
                  }
                }
              }
            }
            SHOW("OS Ver SDK Path is: " << osVerSDKPath.string())
            if (fs::exists(osVerSDKPath)) {
              SHOW("Found OS Ver SDK Path: " << osVerSDKPath.string())
              if (fs::exists(osVerSDKPath / "ucrt")) {
                Maybe<fs::path> candidateUCRTPath;
                if (target.isX86() && target.isArch64Bit()) {
                  candidateUCRTPath = osVerSDKPath / "ucrt" / "x64";
                } else if (target.isX86() && target.isArch32Bit()) {
                  candidateUCRTPath = osVerSDKPath / "ucrt" / "x86";
                } else if (target.isAArch64()) {
                  candidateUCRTPath = osVerSDKPath / "ucrt" / "arm64";
                } else if (target.isARM()) {
                  candidateUCRTPath = osVerSDKPath / "ucrt" / "arm";
                }
                windowsUCRTLibPath = candidateUCRTPath;
              }
              if (fs::exists(osVerSDKPath / "um")) {
                Maybe<fs::path> candidateUMPath;
                if (target.isX86() && target.isArch64Bit()) {
                  candidateUMPath = osVerSDKPath / "um" / "x64";
                } else if (target.isX86() && target.isArch32Bit()) {
                  candidateUMPath = osVerSDKPath / "um" / "x86";
                } else if (target.isAArch64()) {
                  candidateUMPath = osVerSDKPath / "um" / "arm64";
                } else if (target.isARM()) {
                  candidateUMPath = osVerSDKPath / "um" / "arm";
                }
                windowsUMLibPath = candidateUMPath;
              }
            }
          }
        }
      }
    } else {
      irCtx->Error("Could not find the directory " + irCtx->color(windowsPath) +
                       " which is required for finding platform-specific libraries for the target " +
                       irCtx->color(irCtx->clangTargetInfo->getTriple().str()),
                   None);
    }
  } else {
    irCtx->Error("Could not find the " + irCtx->color("toolchain") + " directory. Expected to find the " +
                     irCtx->color("toolchain") + " directory in the directory of installation of qat",
                 None);
  }
  return (findMSVCLibPath ? windowsMSVCLibPath.has_value() : true) &&
         (findATLMFCLibPath ? windowsATLMFCLibPath.has_value() : true) &&
         (findUCRTLibPath ? windowsUCRTLibPath.has_value() : true) &&
         (findUMLibPath ? windowsUMLibPath.has_value() : true);
}

bool Mod::find_windows_sdk_paths(ir::Ctx* irCtx) {
  SHOW("Finding MSVC SDK Path")
  if (windowsMSVCLibPath.has_value() || windowsATLMFCLibPath.has_value() || windowsUCRTLibPath.has_value() ||
      windowsUMLibPath.has_value()) {
    SHOW("Already found MSVC paths")
    return true;
  }
  if (!llvm::Triple(LLVM_HOST_TRIPLE).isOSWindows()) {
    return find_windows_toolchain_libs(irCtx, true, true, true, true);
  }
  Maybe<String> vsWherePath;
  if (fs::exists("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")) {
    vsWherePath = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe";
  }
  if (!vsWherePath.has_value()) {
    auto pathRes = boost::process::search_path("vswhere");
    if (pathRes.string() != "") {
      vsWherePath = pathRes.string();
    }
  }
  auto target = irCtx->clangTargetInfo->getTriple();
  if (!vsWherePath.has_value()) {
    (void)find_windows_toolchain_libs(irCtx, true, true, false, false);
  } else {
    auto vsWhereRes = run_command_get_output(vsWherePath.value(), {"-latest", "-property", "installationPath"});
    if (vsWhereRes.first) {
      irCtx->Error("Running 'vswhere.exe' exited with status code " + std::to_string(vsWhereRes.first) +
                       ". The error output is: " + vsWhereRes.second,
                   None);
    }
    Maybe<String> vsPath;
    if (!vsWhereRes.second.empty()) {
      if (vsWhereRes.second.find('\n') != String::npos) {
        vsPath = vsWhereRes.second.substr(0, vsWhereRes.second.find('\n') - 1);
      } else {
        vsPath = vsWhereRes.second;
      }
    }
    if (!vsPath.has_value()) {
      (void)find_windows_toolchain_libs(irCtx, true, true, false, false);
    } else {
      auto msvcMainPath = fs::path(vsPath.value()) / "VC" / "Tools" / "MSVC";
      // / "14.32.31326" / "bin" / "Hostx64" / "x64";

      if (!fs::exists(msvcMainPath)) {
        (void)find_windows_toolchain_libs(irCtx, true, true, false, false);
      } else {
        // irCtx->Error("The path " + irCtx->color(msvcMainPath.string()) +
        //                  " do not exist. Could not find path of the MSVC SDK",
        //              None);
        SHOW("Found MSVC Root path: " << msvcMainPath.string())
        Maybe<int>      versionMajor;
        Maybe<fs::path> versionPath;
        for (auto it : fs::directory_iterator(msvcMainPath)) {
          if (fs::is_directory(it)) {
            auto dirName = it.path().filename().string();
            if (dirName.find('.') != String::npos) {
              auto majStr = dirName.substr(0, dirName.find('.') - 1);
              if (utils::is_integer(majStr)) {
                auto newVersion = std::stoi(majStr);
                if (versionMajor.has_value() ? (versionMajor.value() < newVersion) : true) {
                  versionMajor = newVersion;
                  versionPath  = it.path();
                }
              }
            }
          }
        }
        if (!versionMajor.has_value()) {
          //   irCtx->Error("Could not find the a version of MSVC in the folder " + irCtx->color(msvcMainPath.string()),
          //                None);
          (void)find_windows_toolchain_libs(irCtx, true, true, false, false);
        } else {
          SHOW("Found MSVC Version path")
          Maybe<fs::path> candidateMSVCLibPath;
          Maybe<fs::path> candidateATLMFCLibPath;
          if (target.isX86() && target.isArch64Bit()) {
            candidateMSVCLibPath   = versionPath.value() / "lib" / "x64";
            candidateATLMFCLibPath = versionPath.value() / "atlmfc" / "lib" / "x64";
          } else if (target.isX86() && target.isArch32Bit()) {
            candidateMSVCLibPath   = versionPath.value() / "lib" / "x86";
            candidateATLMFCLibPath = versionPath.value() / "atlmfc" / "lib" / "x86";
          } else if (target.isAArch64()) {
            candidateMSVCLibPath = versionPath.value() / "lib" / "arm64";
          } else if (target.isARM()) {
            candidateMSVCLibPath = versionPath.value() / "lib" / "arm";
          } else if (target.isWindowsCoreCLREnvironment()) {
            if (target.isArch64Bit()) {
              candidateMSVCLibPath = versionPath.value() / "lib" / "opencore" / "x64";
            } else {
              candidateMSVCLibPath = versionPath.value() / "lib" / "opencore" / "x86";
            }
          }
          if (candidateMSVCLibPath.has_value() && fs::exists(candidateMSVCLibPath.value()) &&
              fs::is_directory(candidateMSVCLibPath.value())) {
            SHOW("Found MSVC Lib Path")
            windowsMSVCLibPath = candidateMSVCLibPath;
          }
          if (candidateATLMFCLibPath.has_value() && fs::exists(candidateATLMFCLibPath.value()) &&
              fs::is_directory(candidateATLMFCLibPath.value())) {
            SHOW("Found ATLMFC Lib Path")
            windowsATLMFCLibPath = candidateATLMFCLibPath;
          }
        }
      }
      //   irCtx->Error("Could not find installation path of the latest MSVC version", None);
    }
  }
  auto windowsKitPath = fs::path("C:/Program Files (x86)/Windows Kits");
  if (!fs::exists(windowsKitPath)) {
    (void)find_windows_toolchain_libs(irCtx, false, false, true, true);
  } else {
    Maybe<fs::path> osVerPath;
    Maybe<float>    osVer;
    for (auto it : fs::directory_iterator(windowsKitPath)) {
      if (fs::is_directory(it)) {
        // FIXME - Rethink for future Windows SDK versions
        if (it.path().filename().string() == "10") {
          if (osVer.has_value() ? (osVer.value() < 10) : true) {
            osVerPath = windowsKitPath / "10";
            osVer     = 10;
          }
        } else if (it.path().filename().string() == "8.1") {
          if (osVer.has_value() ? (osVer.value() < 8.1) : true) {
            osVerPath = windowsKitPath / "8.1";
            osVer     = 8.1;
          }
        } else if (it.path().filename().string() == "7.1") {
          if (osVer.has_value() ? (osVer.value() < 7.1) : true) {
            osVerPath = windowsKitPath / "7.1";
            osVer     = 7.1;
          }
        }
      }
    }
    if (osVerPath.has_value() && fs::exists(osVerPath.value() / "Lib")) {
      SHOW("Found OS Ver Path: " << (osVerPath.value() / "Lib").string())
      std::array<u64, 4> verNums = {0, 0, 0, 0};
      fs::path           osVerSDKPath;
      for (auto it : fs::directory_iterator(osVerPath.value() / "Lib")) {
        if (fs::is_directory(it)) {
          SHOW("In OS Ver main path, iterating " << it.path().string())
          auto  osVersionStr = it.path().filename().string();
          usize dotCount     = 0;
          for (char ch : osVersionStr) {
            if (ch == '.') {
              dotCount++;
            }
          }
          SHOW("Dot count is " << dotCount)
          if (dotCount == 3) {
            auto               firstDot  = osVersionStr.find('.');
            auto               secondDot = osVersionStr.find('.', firstDot + 1);
            auto               thirdDot  = osVersionStr.find('.', secondDot + 1);
            std::array<u64, 4> newVerNum = {std::stoul(osVersionStr.substr(0, firstDot - 1)),
                                            std::stoul(osVersionStr.substr(firstDot + 1, secondDot - 1)),
                                            std::stoul(osVersionStr.substr(secondDot + 1, thirdDot - 1)),
                                            std::stoul(osVersionStr.substr(thirdDot + 1))};
            for (usize i = 0; i < 4; i++) {
              if (newVerNum[i] > verNums[i]) {
                verNums      = newVerNum;
                osVerSDKPath = it.path();
                break;
              } else if (newVerNum[i] == verNums[i]) {
                continue;
              } else {
                break;
              }
            }
          }
        }
      }
      SHOW("OS Ver SDK Path is: " << osVerSDKPath.string())
      if (fs::exists(osVerSDKPath)) {
        SHOW("Found OS Ver SDK Path: " << osVerSDKPath.string())
        if (fs::exists(osVerSDKPath / "ucrt")) {
          Maybe<fs::path> candidateUCRTPath;
          if (target.isX86() && target.isArch64Bit()) {
            candidateUCRTPath = osVerSDKPath / "ucrt" / "x64";
          } else if (target.isX86() && target.isArch32Bit()) {
            candidateUCRTPath = osVerSDKPath / "ucrt" / "x86";
          } else if (target.isAArch64()) {
            candidateUCRTPath = osVerSDKPath / "ucrt" / "arm64";
          } else if (target.isARM()) {
            candidateUCRTPath = osVerSDKPath / "ucrt" / "arm";
          }
          windowsUCRTLibPath = candidateUCRTPath;
        }
        if (fs::exists(osVerSDKPath / "um")) {
          Maybe<fs::path> candidateUMPath;
          if (target.isX86() && target.isArch64Bit()) {
            candidateUMPath = osVerSDKPath / "um" / "x64";
          } else if (target.isX86() && target.isArch32Bit()) {
            candidateUMPath = osVerSDKPath / "um" / "x86";
          } else if (target.isAArch64()) {
            candidateUMPath = osVerSDKPath / "um" / "arm64";
          } else if (target.isARM()) {
            candidateUMPath = osVerSDKPath / "um" / "arm";
          }
          windowsUMLibPath = candidateUMPath;
        }
      } else {
        (void)find_windows_toolchain_libs(irCtx, false, false, true, true);
      }
    } else {
      (void)find_windows_toolchain_libs(irCtx, false, false, true, true);
    }
  }
  return windowsMSVCLibPath.has_value() || windowsATLMFCLibPath.has_value() || windowsUCRTLibPath.has_value() ||
         windowsUMLibPath.has_value();
}

void Mod::bundle_modules(Ctx* ctx) {
  if (!isBundled && (hasMain || ((moduleType == ModuleType::lib) && !parent))) {
    auto& log = Logger::get();
    log->say("Bundling library for module `" + name.value + "` in file " + filePath.string());
    SHOW("Bundling submodules of lib `" << name.value << "`" << " in file " << filePath.string())
    for (auto* sub : submodules) {
      sub->bundle_modules(ctx);
    }
    SHOW("Getting linkable libs")
    auto linkableLibs = getAllLinkableLibs();
    SHOW("Linkable lib count for module " << name.value << " is " << linkableLibs.size())
    auto*       cfg = cli::Config::get();
    Vec<String> cmdOne;
    Vec<String> targetCMD;
    auto        hostTriplet = llvm::Triple(LLVM_HOST_TRIPLE);
    for (auto& lib : linkableLibs) {
      cmdOne.push_back(lib);
    }
    targetCMD.push_back("--target=" + cfg->get_target_triple());
    if (cfg->has_sysroot()) {
      targetCMD.push_back("--sysroot=" + cfg->get_sysroot());
    } else if (ctx->clangTargetInfo->getTriple().isTargetMachineMac()) {
      if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
        targetCMD.push_back("--sysroot=" MACOS_DEFAULT_SDK_PATH);
      } else {
        ctx->Error(
            "The host triplet of the compiler is " + ctx->color(LLVM_HOST_TRIPLE) +
                (hostTriplet.isMacOSX() ? ", and " : " which is not MacOS, but ") + "the target triplet is " +
                ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                " which is identified to be for a MacOS platform. Please provide the --sysroot parameter." +
                (hostTriplet.isTargetMachineMac()
                     ? (" The default value that could have been used for the sysroot is " +
                        ctx->color(MACOS_DEFAULT_SDK_PATH) +
                        " which does not exist. Please make sure that you have Command Line Tools installed by running: " +
                        ctx->color("sudo xcode-select --install"))
                     : ""),
            None);
      }
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      targetCMD.push_back("-nostartfiles");
      targetCMD.push_back("-Wl,--no-entry");
      targetCMD.push_back("-Wl,--export-all");
    }
    std::set<String> objectFiles = getAllObjectPaths();
    Vec<String>      inputFiles;
    for (auto& objPath : objectFiles) {
      inputFiles.push_back(objPath);
    }
    SHOW("Finished input files")
    auto          outNameVal = get_meta_info_for_key("outputName");
    Maybe<String> outputNameValue;
    if (outNameVal.has_value() && outNameVal.value()->get_ir_type()->is_string_slice()) {
      outputNameValue = ir::StringSliceType::value_to_string(outNameVal.value());
    }
    SHOW("Added ll paths of all brought modules")
    if (hasMain) {
      auto outPath = ((cfg->has_output_path() ? cfg->get_output_path() : basePath) / "bin" /
                      filePath.lexically_relative(basePath)
                          .replace_filename(outputNameValue.value_or(name.value))
                          .replace_extension(ctx->clangTargetInfo->getTriple().isOSWindows()
                                                 ? "exe"
                                                 : (ctx->clangTargetInfo->getTriple().isWasm() ? "wasm" : "")))
                         .string();
      std::error_code err;
      fs::create_directories(fs::path(outPath).parent_path(), err);
      if (err) {
        ctx->Error("Could not create the parent directories in the path " +
                       ctx->color(fs::path(outPath).parent_path().string()) +
                       ". The error message was: " + ctx->color(err.message()),
                   None);
      }
      // FIXME - LIB_LINKING
      //   String staticLibStr;
      //   String sharedLibStr;
      //   for (auto const& statLib : staticLibsToLink) {
      //     staticLibStr.append("-l").append(statLib).append(" ");
      //   }
      //   for (auto const& sharedLib : sharedLibsToLink) {
      //     sharedLibStr.append("-l").append(sharedLib).append(" ");
      //   }
      (void)find_clang_path(ctx);

      Vec<String> staticArgs;
      staticArgs.push_back("--verbose");
      if (linkPthread) {
        if (ctx->clangTargetInfo->getTriple().isOSLinux() || ctx->clangTargetInfo->getTriple().isOSCygMing()) {
          staticArgs.push_back("-pthread");
        }
      }
      staticArgs.push_back("-o");
      staticArgs.push_back(outPath);
      staticArgs.insert(staticArgs.end(), cmdOne.begin(), cmdOne.end());
      // FIXME - LIB_LINKING .append(staticLibStr)
      staticArgs.insert(staticArgs.end(), targetCMD.begin(), targetCMD.end());
      staticArgs.insert(staticArgs.end(), inputFiles.begin(), inputFiles.end());

      Vec<String> sharedArgs;
      if (linkPthread) {
        if (!ctx->clangTargetInfo->getTriple().isWindowsMSVCEnvironment()) {
          sharedArgs.push_back("-pthread");
        }
      }
      sharedArgs.push_back("-o");
      sharedArgs.push_back(outPath);
      sharedArgs.insert(sharedArgs.end(), cmdOne.begin(), cmdOne.end());
      // FIXME - LIB_LINKING .append(sharedLibStr)
      sharedArgs.insert(sharedArgs.end(), targetCMD.begin(), targetCMD.end());
      sharedArgs.insert(sharedArgs.end(), inputFiles.begin(), inputFiles.end());

      if (cfg->should_build_static()) {
        auto cmdRes = run_command_get_stderr(usableClangPath.value(), staticArgs);
        if (cmdRes.first) {
          ctx->Error("Statically compiling & linking executable failed: " + ctx->color(filePath.string()) +
                         ". The error output is\n" + cmdRes.second,
                     None);
        }
      } else if (cfg->should_build_shared()) {
        auto cmdRes = run_command_get_stderr(usableClangPath.value(), sharedArgs);
        if (cmdRes.first) {
          ctx->Error("Dynamically compiling & linking executable failed: " + ctx->color(filePath.string()) +
                         ". The error output is\n" + cmdRes.second,
                     None);
        }
      }
      ctx->add_exe_path(fs::absolute(outPath).lexically_normal());
      if (cfg->export_code_metadata()) {
        std::ifstream file(filePath, std::ios::binary);
        auto          fsize = file.tellg();
        file.seekg(0, std::ios::end);
        fsize = file.tellg() - fsize;
        ctx->add_binary_size(fsize);
        file.close();
      }
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
      auto getNameForLib = [&](Maybe<String> outName) {
        auto triple = ctx->clangTargetInfo->getTriple();
        if (triple.isOSWindows()) {
          return outName.value_or(name.value);
        } else {
          return "lib" + outName.value_or(name.value);
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
      auto outPath = fs::absolute((cfg->has_output_path() ? cfg->get_output_path() : basePath) / "libs" /
                                  filePath.lexically_relative(basePath).replace_filename(
                                      getNameForLib(outputNameValue).append(getExtension())))
                         .string();
      auto outPathShared = fs::absolute((cfg->has_output_path() ? cfg->get_output_path() : basePath) / "libs" /
                                        filePath.lexically_relative(basePath).replace_filename(
                                            getNameForLib(outputNameValue).append(getExtensionShared())))
                               .string();
      // NOTE - Assuming both static and shared libraries live in the same directory
      std::error_code err;
      fs::create_directories(fs::path(outPath).parent_path(), err);
      if (err) {
        ctx->Error("Could not create the parent directories in the path " +
                       ctx->color(fs::path(outPath).parent_path().string()) +
                       ". The error message was: " + ctx->color(err.message()),
                   None);
      }
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
        if (linkPthread) {
          allArgs.push_back("-pthread");
          allArgsShared.push_back("-pthread");
        }
        if (cfg->is_build_mode_release()) {
          allArgs.push_back("--strip-debug");
          allArgsShared.push_back("--strip-debug");
        }
        allArgsShared.push_back("-Bdynamic");
        auto sysRootStr = String("--sysroot=\"").append(cfg->has_sysroot() ? cfg->get_sysroot() : "").append("\"");
        if (cfg->has_sysroot()) {
          allArgs.push_back(sysRootStr.c_str());
          allArgsShared.push_back(sysRootStr.c_str());
        } else if (!triple_is_equivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("Please provide the --sysroot parameter to build the library for the MinGW target triple " +
                         ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " which is not compatible with the compiler's host triplet " +
                         ctx->color(ctx->clangTargetInfo->getTriple().getTriple()),
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library MingW")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::MinGW, &lld::mingw::link}});
          if (result.retCode != 0) {
            String staticCmdStr;
            for (auto arg : allArgs) {
              staticCmdStr += String(arg) + " ";
            }
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM MinGW' linker failed with status code " + std::to_string(result.retCode) +
                           "\nThe command was: " + staticCmdStr + "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->should_build_shared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::MinGW, &lld::mingw::link}});
          if (resultShared.retCode != 0) {
            String sharedCmdStr;
            for (auto arg : allArgsShared) {
              sharedCmdStr += String(arg) + " ";
            }
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM MinGW' linker failed with status code " +
                           std::to_string(resultShared.retCode) + "\nThe command was: " + sharedCmdStr +
                           "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSWindows()) {
        /**
         *  Windows Linker
         */
        auto msvcRes      = find_windows_sdk_paths(ctx);
        auto outSharedArg = "/OUT:" + outPathShared;

        Vec<String>      allArgs{"/OUT:" + outPath};
        Vec<const char*> allArgsShared{
            "lld.link", "-flavor", "link", "/DLL", outSharedArg.c_str(),
        };

        String libPathMSVC;
        String libPathATLMFC;
        String libPathUCRT;
        String libPathUM;
        if (windowsMSVCLibPath.has_value()) {
          libPathMSVC = "/LIBPATH:\"" + windowsMSVCLibPath.value().string() + "\"";
          allArgs.push_back(libPathMSVC);
          allArgsShared.push_back(libPathMSVC.c_str());
        }
        if (windowsATLMFCLibPath.has_value()) {
          libPathATLMFC = "/LIBPATH:\"" + windowsATLMFCLibPath.value().string() + "\"";
          allArgs.push_back(libPathATLMFC);
          allArgsShared.push_back(libPathATLMFC.c_str());
        }
        if (windowsUCRTLibPath.has_value()) {
          libPathUCRT = "/LIBPATH:\"" + windowsUCRTLibPath.value().string() + "\"";
          allArgs.push_back(libPathUCRT);
          allArgsShared.push_back(libPathUCRT.c_str());
        }
        if (windowsUMLibPath.has_value()) {
          libPathUM = "/LIBPATH:\"" + windowsUMLibPath.value().string() + "\"";
          allArgs.push_back(libPathUM);
          allArgsShared.push_back(libPathUM.c_str());
        }

        if (cfg->is_build_mode_release()) {
          allArgsShared.push_back("/RELEASE");
        } else {
          allArgsShared.push_back("/DEBUG");
        }
        auto machineStr = "/MACHINE:" + windowsTripleToMachine(ctx->clangTargetInfo->getTriple());
        if (cfg->has_target_triple()) {
          allArgs.push_back(machineStr);
          allArgsShared.push_back(machineStr.c_str());
        }
        auto libPathStr = "/LIBPATH:\"" + (cfg->has_sysroot() ? cfg->get_sysroot() : "") + "\"";
        if (cfg->has_sysroot()) {
          allArgs.push_back(libPathStr);
          allArgsShared.push_back(libPathStr.c_str());
        } else if (!triple_is_equivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("The target triplet " + ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " is not compatible with the compiler's host triplet " + ctx->color(hostTriplet.getTriple()) +
                         ". Please provide the " + ctx->color("--sysroot") +
                         " parameter with the path to the SDK for the target platform",
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj);
          allArgsShared.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          auto llvmLibPath = boost::process::search_path("llvm-ar-17");
          if (llvmLibPath.string() == "") {
            ctx->Error("Could not find " + ctx->color("llvm-ar-17") + " on the PATH. " + ctx->color("llvm-ar-17") +
                           " is the renamed version of " + ctx->color("llvm-ar") +
                           " utility, bundled with the QAT installation. Please check your installation of qat",
                       None);
          }
          SHOW("Linking Static Library Windows")
          auto llvmLibRes = run_command_get_stderr(llvmLibPath.string(), allArgs);
          if (llvmLibRes.first) {
            auto staticCmdStr = llvmLibPath.string() + " ";
            for (auto arg : allArgs) {
              staticCmdStr += arg + " ";
            }
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM Lib' program failed with status code " + std::to_string(llvmLibRes.first) +
                           "\nThe command was: " + staticCmdStr + "\nThe error output is: " + llvmLibRes.second,
                       None);
          }
        }
        if (cfg->should_build_shared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::WinLink, &lld::coff::link}});
          if (resultShared.retCode != 0) {
            String sharedCmdStr;
            for (auto arg : allArgsShared) {
              sharedCmdStr += String(arg) + " ";
            }
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM Link (MSVC)' linker failed with status code " +
                           std::to_string(resultShared.retCode) + "\nThe command was: " + sharedCmdStr +
                           "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatELF()) {
        /**
         *  ELF Linker
         */
        SHOW("Outpath is " << outPath)
        Vec<const char*> allArgs{"ld.lld", "-flavor", "gnu", "-r", "-o", outPath.c_str()};
        Vec<const char*> allArgsShared{"ld.lld", "-flavor", "gnu", "-r", "-o", outPathShared.c_str()};
        Vec<String>      folderPaths;
        Vec<String>      staticPaths;
        if (cfg->is_build_mode_release()) {
          allArgs.push_back("--strip-debug");
          allArgsShared.push_back("--strip-debug");
        }
        allArgsShared.push_back("-Bdynamic");
        String libPathStr =
            String("--library-path=\"").append(cfg->has_sysroot() ? cfg->get_sysroot() : "").append("\"");
        if (cfg->has_sysroot()) {
          allArgs.push_back(libPathStr.c_str());
          allArgsShared.push_back(libPathStr.c_str());
        } else if (!triple_is_equivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("The target triple " + ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " is not compatible with the compiler's host triplet " + ctx->color(hostTriplet.getTriple()) +
                         ". Please provide the --sysroot parameter with the path to the SDK for the target platform",
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library ELF")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Gnu, &lld::elf::link}});
          if (result.retCode != 0 || !stdErrStr.empty()) {
            String staticCmdStr;
            for (auto arg : allArgs) {
              staticCmdStr += String(arg) + " ";
            }
            SHOW("Linker failed")
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM lld (ELF)' linker failed with status code " +
                           std::to_string(result.retCode) + "\nThe command was: " + staticCmdStr +
                           "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
          SHOW("Linker done for " << name.value)
        }
        if (cfg->should_build_shared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::Gnu, &lld::elf::link}});
          if (resultShared.retCode != 0) {
            String sharedCmdStr;
            for (auto arg : allArgsShared) {
              sharedCmdStr += String(arg) + " ";
            }
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM lld (ELF)' linker failed with status code " +
                           std::to_string(resultShared.retCode) + "\nThe command was: " + sharedCmdStr +
                           "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else if (ctx->clangTargetInfo->getTriple().isOSBinFormatWasm()) {
        /**
         *  WASM Linker
         */
        Vec<const char*> allArgs{"wasm-ld", "-flavor", "wasm", "--export-dynamic", "-o", outPath.c_str()};
        if (cfg->has_sysroot()) {
          allArgs.push_back(String("--library-path=\"").append(cfg->get_sysroot()).append("\"").c_str());
        } else if (!hostTriplet.isWasm()) {
          ctx->Error("Please provide the --sysroot parameter to build the library for the WASM target " +
                         ctx->color(ctx->clangTargetInfo->getTriple().getTriple()),
                     None);
        }
        Vec<String> staticCmdList;
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library WASM")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Wasm, &lld::wasm::link}});
          if (result.retCode != 0) {
            String staticCmdStr;
            for (auto arg : allArgs) {
              staticCmdStr += String(arg) + " ";
            }
            ctx->Error("Building static WASM library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM WASM' linker failed with status code " + std::to_string(result.retCode) +
                           +"\nThe command was: " + staticCmdStr + "\nThe linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->should_build_shared()) {
          ctx->Error("Cannot build shared libraries for WASM for now. Please see link: " +
                         ctx->color("https://github.com/WebAssembly/tool-conventions/blob/main/DynamicLinking.md"),
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
        allArgsShared.push_back("-dynamic");
        allArgs.push_back("-arch");
        allArgsShared.push_back("-arch");
        auto archStr = (cfg->has_target_triple() ? ctx->clangTargetInfo->getTriple() : llvm::Triple(LLVM_HOST_TRIPLE))
                           .getArchName()
                           .str();
        allArgs.push_back(archStr.c_str());
        allArgsShared.push_back(archStr.c_str());
        auto sysrootStr = cfg->has_sysroot() ? cfg->get_sysroot() : "";
        if (cfg->has_sysroot()) {
          allArgs.push_back("-syslibroot");
          allArgs.push_back(sysrootStr.c_str());
          allArgsShared.push_back("-syslibroot");
          allArgsShared.push_back(sysrootStr.c_str());
        } else if (ctx->clangTargetInfo->getTriple().isMacOSX()) {
          if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
            sysrootStr = String(MACOS_DEFAULT_SDK_PATH);
            allArgs.push_back("-syslibroot");
            allArgs.push_back(sysrootStr.c_str());
            allArgsShared.push_back("-syslibroot");
            allArgsShared.push_back(sysrootStr.c_str());
          } else {
            ctx->Error("The compiler's host triplet is " + ctx->color(LLVM_HOST_TRIPLE) +
                           (hostTriplet.isMacOSX() ? "" : " which is not MacOS") + ", but the target triplet is " +
                           ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                           " which is identified to be for a MacOS platform. Please provide the --sysroot parameter." +
                           (hostTriplet.isTargetMachineMac()
                                ? (" The default value that could have been used for the sysroot is " +
                                   ctx->color(MACOS_DEFAULT_SDK_PATH) + " which does not exist")
                                : ""),
                       None);
          }
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library Mac")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Darwin, &lld::macho::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM ld64 (MacOS)' linker failed with status code " +
                           ctx->color(std::to_string(result.retCode)) + ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
        if (cfg->should_build_shared()) {
          linkerStdOut.flush();
          linkerStdErr.flush();
          SHOW("Linking Shared Library Mac")
          auto resultShared = lld::lldMain(llvm::ArrayRef<const char*>(allArgsShared), linkerStdOut, linkerStdErr,
                                           {{lld::Darwin, &lld::macho::link}});
          if (resultShared.retCode != 0) {
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " using the 'LLVM ld64 (MacOS)' linker failed with status code " +
                           ctx->color(std::to_string(resultShared.retCode)) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else {
        if (cfg->has_linker_path()) {
          auto        cmd = cfg->get_linker_path();
          Vec<String> cmdArgs;
          cmdArgs.push_back("-o");
          cmdArgs.push_back(outPath);
          cmdArgs.insert(cmdArgs.end(), objectFiles.begin(), objectFiles.end());
          auto cmdRes = run_command_get_output(cmd, cmdArgs);
          if (cmdRes.first) {
            ctx->Error("Building library failed for module " + ctx->color(name.value) + " in " +
                           ctx->color(filePath.string()) + ". The output is\n" + cmdRes.second,
                       None);
          }
        } else {
          ctx->Error("Could not find the linker to be used for the target " +
                         ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                         ". Please provide the --linker argument with the path to the linker to use",
                     None);
        }
      }
    }
    isBundled = true;
  }
}

fs::path Mod::get_resolved_output_path(const String& extension, Ctx* ctx) {
  auto* config = cli::Config::get();
  auto  fPath  = filePath;
  auto  out    = fPath.replace_extension(extension);
  if (config->has_output_path()) {
    out = (config->get_output_path() / filePath.lexically_relative(basePath).remove_filename());
    std::error_code errorCode;
    fs::create_directories(out, errorCode);
    if (!errorCode) {
      out = out / fPath.filename();
    } else {
      ctx->Error("Could not create the parent directory of the output files with path: " + ctx->color(out.string()) +
                     " with error: " + errorCode.message(),
                 None);
    }
  }
  return out;
}

void Mod::export_json_from_ast(Ctx* ctx) {
  if ((moduleType == ModuleType::file) || rootLib) {
    auto*          cfg    = cli::Config::get();
    auto           result = Json();
    Vec<JsonValue> contents;
    for (auto* node : nodes) {
      contents.push_back(node->to_json());
    }
    result["contents"] = contents;
    std::fstream jsonStream;
    auto         jsonPath =
        (cfg->has_output_path() ? cfg->get_output_path() : basePath) / "AST" /
        filePath.lexically_relative(basePath).replace_filename(filePath.filename().string()).replace_extension("json");
    std::error_code errorCode;
    fs::create_directories(jsonPath.parent_path(), errorCode);
    if (!errorCode) {
      jsonStream.open(jsonPath, std::ios_base::out);
      if (jsonStream.is_open()) {
        jsonStream << result;
        jsonStream.close();
      } else {
        ctx->Error("Output file could not be opened for writing the JSON representation",
                   {get_parent_file()->filePath});
      }
    } else {
      ctx->Error("Could not create parent directories for the JSON file for exporting AST",
                 {get_parent_file()->filePath});
    }
  } else {
    SHOW("Module type not suitable for exporting AST")
  }
}

llvm::Module* Mod::get_llvm_module() const { return llvmModule; }

void Mod::link_intrinsic(IntrinsicID intr) {
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

void Mod::link_native(NativeUnit nval) {
  // FIXME - Use integer widths according to the specification and not the same
  // on all platforms
  llvm::LLVMContext& llCtx = llvmModule->getContext();
  switch (nval) {
    case NativeUnit::printf: {
      if (!llvmModule->getFunction("printf")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, true),
                               llvm::GlobalValue::LinkageTypes::ExternalLinkage, "printf", llvmModule);
      }
      break;
    }
    case NativeUnit::malloc: {
      if (!llvmModule->getFunction("malloc")) {
        SHOW("Creating malloc function")
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(),
                                                       {llvm::Type::getInt64Ty(llCtx)}, false),
                               llvm::GlobalValue::ExternalLinkage, "malloc", llvmModule);
      }
      break;
    }
    case NativeUnit::free: {
      if (!llvmModule->getFunction("free")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, false),
                               llvm::GlobalValue::LinkageTypes::ExternalLinkage, "free", llvmModule);
      }
      break;
    }
    case NativeUnit::realloc: {
      if (!llvmModule->getFunction("realloc")) {
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(),
                                    {llvm::Type::getInt8Ty(llCtx)->getPointerTo(), llvm::Type::getInt64Ty(llCtx)},
                                    false),
            llvm::GlobalValue::LinkageTypes::ExternalLinkage, "realloc", llvmModule);
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
            llvm::GlobalValue::LinkageTypes::ExternalLinkage, "pthread_create", llvmModule);
        linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadJoin: {
      if (!llvmModule->getFunction("pthread_join")) {
        llvm::Type* pthreadTy = llvm::Type::getInt64Ty(llCtx);
        llvm::Type* voidPtrTy = llvm::Type::getInt8Ty(llCtx)->getPointerTo();
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(llCtx), {pthreadTy, voidPtrTy->getPointerTo()}, false),
            llvm::GlobalValue::LinkageTypes::ExternalLinkage, "pthread_join", llvmModule);
        linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadExit: {
      if (!llvmModule->getFunction("pthread_exit")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, false),
                               llvm::GlobalValue::LinkageTypes::ExternalLinkage, "pthread_exit", llvmModule);
        linkPthread = true;
      }
      break;
    }
    case NativeUnit::windowsExitThread: {
      if (!llvmModule->getGlobalVariable("__imp_ExitThread")) {
        new llvm::GlobalVariable(
            *llvmModule,
            llvm::PointerType::get(
                llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {llvm::Type::getInt32Ty(llCtx)}, false), 0u),
            true, llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, "__imp_ExitThread", nullptr,
            llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, None, true);
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
            llvm::GlobalValue::LinkageTypes::ExternalLinkage, "pthread_attr_init", llvmModule);
        linkPthread = true;
      }
      break;
    }
  }
}

Json Mod::to_json() const {
  // FIXME - Change
  return Json()._("name", name);
}

} // namespace qat::ir
