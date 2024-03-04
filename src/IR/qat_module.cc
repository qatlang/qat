#include "./qat_module.hpp"
#include "../ast/node.hpp"
#include "../cli/config.hpp"
#include "../show.hpp"
#include "../utils/logger.hpp"
#include "../utils/pstream/pstream.h"
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
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace qat::ir {

Vec<Mod*>     Mod::allModules{};
Vec<fs::path> Mod::usableNativeLibPaths{};

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
    } else if (phaseToChildrenPartial.has_value() && phaseToCompletion.value() == nextPhase) {
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

bool Mod::hasFileModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

bool Mod::hasFolderModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if (mod->moduleType == ModuleType::folder) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return true;
      }
    }
  }
  return false;
}

Mod* Mod::getFileModule(const fs::path& fPath) {
  for (auto* mod : allModules) {
    if ((mod->moduleType == ModuleType::file) || mod->rootLib) {
      if (fs::equivalent(mod->filePath, fPath)) {
        return mod;
      }
    }
  }
  return nullptr;
}

Mod* Mod::getFolderModule(const fs::path& fPath) {
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

Mod* Mod::Create(const Identifier& name, const fs::path& filepath, const fs::path& basePath, ModuleType type,
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

Function* Mod::create_function(const Identifier& name, Type* returnType, Vec<Argument> args, bool isVariadic,
                               const FileRange& fileRange, const VisibilityInfo& visibility,
                               Maybe<llvm::GlobalValue::LinkageTypes> linkage, Ctx* ctx) {
  SHOW("Creating IR function")
  auto nmUnits = get_link_names();
  nmUnits.addUnit(LinkNameUnit(name.value, LinkUnitType::function, {}), None);
  auto* fun = Function::Create(this, name, nmUnits, {/* Generics */}, ir::ReturnType::get(returnType), std::move(args),
                               isVariadic, fileRange, visibility, ctx, linkage);
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

Mod* Mod::CreateSubmodule(Mod* parent, fs::path filepath, fs::path basePath, Identifier sname, ModuleType type,
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

Mod* Mod::CreateFileMod(Mod* parent, fs::path filepath, fs::path basePath, Identifier fname, Vec<ast::Node*> nodes,
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

Mod* Mod::CreateRootLib(Mod* parent, fs::path filepath, fs::path basePath, Identifier fname, Vec<ast::Node*> nodes,
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
    moduleInitialiser = ir::Function::Create(this, Identifier("module'initialiser'" + utils::unique_id(), {filePath}),
                                             None, {/* Generics */}, ir::ReturnType::get(ir::VoidType::get(ctx->llctx)),
                                             {}, false, name.range, VisibilityInfo::pub(), ctx);
    auto* entry       = new ir::Block(moduleInitialiser, nullptr);
    entry->set_active(ctx->builder);
  }
  return moduleInitialiser;
}

void Mod::add_non_const_global_counter() { nonConstantGlobals++; }

bool Mod::should_call_initialiser() const { return nonConstantGlobals != 0; }

void Mod::addNamedSubmodule(const Identifier& sname, const String& filename, ModuleType type,
                            const VisibilityInfo& visib_info, Ctx* ctx) {
  SHOW("Creating submodule: " << sname.value)
  active = CreateSubmodule(this, filename, basePath.string(), sname, type, visib_info, ctx);
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

void Mod::bring_generic_struct_type(GenericCoreType* gCTy, const VisibilityInfo& visib, Maybe<Identifier> bName) {
  if (bName.has_value()) {
    broughtGenericCoreTypes.push_back(Brought<GenericCoreType>(bName.value(), gCTy, visib));
  } else {
    broughtGenericCoreTypes.push_back(Brought<GenericCoreType>(gCTy, visib));
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

GenericCoreType* Mod::get_generic_struct_type(const String& name, const AccessInfo& reqInfo) {
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
      ctx->Error("Error while writing the LLVM IR to the file " + ctx->color(llPath), None);
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

void Mod::compile_to_object(Ctx* ctx) {
  if (!isCompiledToObject) {
    auto& log = Logger::get();
    log->say("Compiling module `" + name.value + "` from file " + filePath.string());
    auto*  cfg = cli::Config::get();
    String compileCommand(cfg->has_clang_path() ? (fs::absolute(cfg->get_clang_path()).string() + " ") : "clang ");
    if (cfg->should_build_shared()) {
      compileCommand.append("-fPIC -c -mllvm -enable-matrix ");
    } else {
      compileCommand.append("-c -mllvm -enable-matrix ");
    }
    if (linkPthread) {
      compileCommand += "-pthread ";
    }
    compileCommand.append("--target=").append(ctx->clangTargetInfo->getTriple().getTriple()).append(" ");
    if (cfg->has_sysroot()) {
      compileCommand.append("--sysroot=").append(cfg->get_sysroot()).append(" ");
    }
    if (ctx->clangTargetInfo->getTriple().isWasm()) {
      // -Wl,--import-memory
      compileCommand.append("-nostartfiles -Wl,--no-entry -Wl,--export-all ");
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
    compileCommand.append(llPath.string()).append(" -o ").append(objectFilePath.value().string());
    SHOW("Compile command is " << compileCommand)
    log->say("Compile command is: " + compileCommand);
    redi::ipstream proc(compileCommand, redi::pstreams::pstderr);
    String         errMessage;
    String         line;
    while (std::getline(proc.err(), line)) {
      errMessage += line += "\n";
    }
    while (!proc.rdbuf()->exited()) {
    }
    if (proc.rdbuf()->status()) {
      ctx->Error("Could not compile the LLVM file: " + ctx->color(filePath.string()) + "\n" + errMessage, None);
    }
    isCompiledToObject = true;
  }
}

std::set<String> Mod::getAllObjectPaths() const {
  SHOW("GetAllObjectPaths for `" << name.value << "` in " << filePath.string())
  std::set<String> result;
  if (objectFilePath.has_value()) {
    SHOW("Object path has value")
    result.insert(objectFilePath.value());
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

void Mod::bundle_modules(Ctx* ctx) {
  if (!isBundled && (hasMain || ((moduleType == ModuleType::lib) && !parent))) {
    auto& log = Logger::get();
    log->say("Bundling library for module `" + name.value + "` in file " + filePath.string());
    SHOW("Bundling submodules of lib `" << name.value << "`"
                                        << " in file " << filePath.string())
    log->finish_output();
    for (auto* sub : submodules) {
      sub->bundle_modules(ctx);
    }
    SHOW("Getting linkable libs")
    auto linkableLibs = getAllLinkableLibs();
    SHOW("Linkable lib count for module " << name.value << " is " << linkableLibs.size())
    auto*  cfg = cli::Config::get();
    String cmdOne;
    String targetCMD;
    if (linkPthread) {
      cmdOne.append("-pthread ");
    }
    auto hostTriplet = llvm::Triple(LLVM_HOST_TRIPLE);
    SHOW("Created triple")
    for (auto& lib : linkableLibs) {
      cmdOne.append(" ").append(lib).append(" ");
    }
    SHOW("Appended all linkable libs")
    targetCMD.append("--target=").append(cfg->get_target_triple()).append(" ");

#define MACOS_DEFAULT_SDK_PATH "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"

    if (cfg->has_sysroot()) {
      targetCMD.append("--sysroot=").append(cfg->get_sysroot()).append(" ");
    } else if (ctx->clangTargetInfo->getTriple().isTargetMachineMac()) {
      if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
        targetCMD.append("--sysroot=").append("\"" MACOS_DEFAULT_SDK_PATH "\" ");
      } else {
        ctx->Error(
            "The host triplet of the compiler is " + ctx->color(LLVM_HOST_TRIPLE) +
                (hostTriplet.isMacOSX() ? "" : " which is not MacOS") + ", but the target triplet is " +
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
      targetCMD.append("-nostartfiles -Wl,--no-entry -Wl,--export-all ");
    }
    std::set<String> objectFiles = getAllObjectPaths();
    String           inputFiles  = "";
    for (auto& objPath : objectFiles) {
      inputFiles.append(objPath);
      inputFiles.append(" ");
    }
    SHOW("Finished input files")
    // log->finish_output();
    auto          outNameVal = get_meta_info_for_key("outputName");
    Maybe<String> outputNameValue;
    if (outNameVal.has_value() && outNameVal.value()->get_ir_type()->is_string_slice()) {
      outputNameValue = ir::StringSliceType::value_to_string(outNameVal.value());
    }
    SHOW("Added ll paths of all brought modules")
    if (hasMain) {
      auto outPath = ((cfg->has_output_path() ? cfg->get_output_path() : basePath) /
                      filePath.lexically_relative(basePath)
                          .replace_filename(outputNameValue.value_or(name.value))
                          .replace_extension(ctx->clangTargetInfo->getTriple().isOSWindows()
                                                 ? "exe"
                                                 : (ctx->clangTargetInfo->getTriple().isWasm() ? "wasm" : "")))
                         .string();
      ctx->add_exe_path(fs::absolute(outPath).lexically_normal());
      // FIXME - LIB_LINKING
      //   String staticLibStr;
      //   String sharedLibStr;
      //   for (auto const& statLib : staticLibsToLink) {
      //     staticLibStr.append("-l").append(statLib).append(" ");
      //   }
      //   for (auto const& sharedLib : sharedLibsToLink) {
      //     sharedLibStr.append("-l").append(sharedLib).append(" ");
      //   }
      auto staticCommand = String(cfg->has_clang_path() ? cfg->get_clang_path() : "clang")
                               .append(" -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
                               // FIXME - LIB_LINKING
                               //    .append(staticLibStr)
                               .append(targetCMD)
                               .append(inputFiles);
      auto sharedCommand = String(cfg->has_clang_path() ? cfg->get_clang_path() : "clang")
                               .append(" -o ")
                               .append(outPath)
                               .append(" ")
                               .append(cmdOne)
                               // FIXME - LIB_LINKING
                               //    .append(sharedLibStr)
                               .append(targetCMD)
                               .append(inputFiles);
      if (cfg->should_build_static()) {
        SHOW("Static Build Command :: " << staticCommand)
        redi::ipstream proc(staticCommand, redi::pstreams::pstderr);
        String         errMessage;
        String         line;
        while (std::getline(proc.err(), line)) {
          errMessage += line += "\n";
        }
        while (!proc.rdbuf()->exited()) {
        }
        if (proc.rdbuf()->status()) {
          ctx->Error("Statically linking & compiling executable failed: " + filePath.string() + "\n" + errMessage,
                     None);
        }
      } else if (cfg->should_build_shared()) {
        SHOW("Dynamic Build Command :: " << sharedCommand)
        redi::ipstream proc(sharedCommand, redi::pstreams::pstderr);
        String         errMessage;
        String         line;
        while (std::getline(proc.err(), line)) {
          errMessage += line += "\n";
        }
        while (!proc.rdbuf()->exited()) {
        }
        if (proc.rdbuf()->status()) {
          ctx->Error("Dynamically linking & compiling executable failed: " + filePath.string() + "\n" + errMessage,
                     None);
        }
      }
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
      auto outPath = fs::absolute((cfg->has_output_path() ? cfg->get_output_path() : basePath) /
                                  filePath.lexically_relative(basePath).replace_filename(
                                      outputNameValue.value_or(get_writable_name()).append(getExtension())))
                         .string();
      auto outPathShared = fs::absolute((cfg->has_output_path() ? cfg->get_output_path() : basePath) /
                                        filePath.lexically_relative(basePath).replace_filename(
                                            outputNameValue.value_or(get_writable_name()).append(getExtensionShared())))
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
        // FIXME - LIB_LINKING
        // for (auto& statLib : staticLibsToLink) {
        //   staticCmdList.push_back("-l" + statLib);
        //   allArgs.push_back(staticCmdList.back().c_str());
        // }
        // for (auto& sharedLib : sharedLibsToLink) {
        //   sharedCmdList.push_back("-l" + sharedLib);
        //   allArgsShared.push_back(sharedCmdList.back().c_str());
        // }
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
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " failed with return code " + std::to_string(result.retCode) +
                           ". The linker's error output is: " + stdErrStr,
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
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
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
        if (cfg->is_build_mode_release()) {
          allArgs.push_back("/RELEASE");
          allArgsShared.push_back("/RELEASE");
        } else {
          allArgs.push_back("/DEBUG");
          allArgsShared.push_back("/DEBUG");
        }
        allArgsShared.push_back("/DLL");
        auto machineStr = "/MACHINE:" + windowsTripleToMachine(ctx->clangTargetInfo->getTriple());
        if (cfg->has_target_triple()) {
          allArgs.push_back(machineStr.c_str());
          allArgsShared.push_back(machineStr.c_str());
        }
        auto libPathStr = "/LIBPATH:\"" + (cfg->has_sysroot() ? cfg->get_sysroot() : "") + "\"";
        if (cfg->has_sysroot()) {
          allArgs.push_back(libPathStr.c_str());
          allArgsShared.push_back(libPathStr.c_str());
        } else if (!triple_is_equivalent(hostTriplet, ctx->clangTargetInfo->getTriple())) {
          ctx->Error("The target triplet " + ctx->color(ctx->clangTargetInfo->getTriple().getTriple()) +
                         " is not compatible with the compiler's host triplet " + ctx->color(hostTriplet.getTriple()) +
                         ". Please provide the --sysroot parameter with the path to the SDK for the target platform",
                     None);
        }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
          allArgsShared.push_back(obj.c_str());
        }
        // FIXME - LIB_LINKING
        // for (auto& statLib : staticLibsToLink) {
        //   allArgs.push_back(statLib.c_str());
        // }
        // for (auto& sharedLib : sharedLibsToLink) {
        //   allArgsShared.push_back(sharedLib.c_str());
        // }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library Windows")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::WinLink, &lld::coff::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
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
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " failed with return code " + std::to_string(resultShared.retCode) +
                           ". The linker's error output is: " + stdErrStr,
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
        // Vec<String>      staticCmdList;
        // Vec<String>      sharedCmdList;

        Vec<String> folderPaths;
        Vec<String> staticPaths;
        // FIXME - LIB_LINKING
        // for (auto& statLib : staticLibsToLink) {
        //   staticCmdList.push_back("-l" + statLib);
        //   allArgs.push_back(staticCmdList.back().c_str());
        // }
        // for (auto& sharedLib : sharedLibsToLink) {
        //   sharedCmdList.push_back("-l" + sharedLib);
        //   allArgsShared.push_back(sharedCmdList.back().c_str());
        // }
        if (cfg->is_build_mode_release()) {
          allArgs.push_back("--strip-debug");
          allArgsShared.push_back("--strip-debug");
        }
        // for (auto folder : usableNativeLibPaths) {
        //   folderPaths.push_back("-L" + folder.string());
        //   allArgs.push_back(folderPaths.back().c_str());
        //   allArgsShared.push_back(folderPaths.back().c_str());
        // }
        // for (auto linkLib : linkableLibs) {
        //   auto staticPath = findStaticLibraryPath(linkLib.substr(2));
        //   if (staticPath.has_value()) {
        //     staticPaths.push_back("-l" + staticPath.value().string());
        //     allArgs.push_back(staticPaths.back().c_str());
        //   } else {
        //     allArgs.push_back(linkLib.c_str());
        //   }
        //   allArgsShared.push_back(linkLib.c_str());
        // }
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
            SHOW("Linker failed")
            ctx->Error("Building static library for module " + ctx->color(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
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
            ctx->Error("Building shared library for module " + ctx->color(filePath.string()) +
                           " failed. The linker's error can be found in " + stdErrStr,
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
        // FIXME - LIB_LINKING
        // for (auto& statLib : staticLibsToLink) {
        //   staticCmdList.push_back("-l" + statLib);
        //   allArgs.push_back(staticCmdList.back().c_str());
        // }
        for (auto& obj : objectFiles) {
          allArgs.push_back(obj.c_str());
        }
        if (cfg->should_build_static()) {
          SHOW("Linking Static Library WASM")
          auto result = lld::lldMain(llvm::ArrayRef<const char*>(allArgs), linkerStdOut, linkerStdErr,
                                     {{lld::Wasm, &lld::wasm::link}});
          if (result.retCode != 0) {
            ctx->Error("Building static WASM library for module " + ctx->color(filePath.string()) +
                           " failed. The linker's error output is: " + stdErrStr,
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
        // FIXME - LIB_LINKING
        // for (auto& statLib : staticLibsToLink) {
        //   staticCmdList.push_back("-l" + statLib);
        //   allArgs.push_back(staticCmdList.back().c_str());
        // }
        // for (auto& sharedLib : sharedLibsToLink) {
        //   sharedCmdList.push_back("-l" + sharedLib);
        //   allArgsShared.push_back(sharedCmdList.back().c_str());
        // }
        allArgsShared.push_back("-dynamic");
        if (cfg->has_target_triple()) {
          allArgs.push_back(("-arch=" + ctx->clangTargetInfo->getTriple().getArchName().str()).c_str());
          allArgsShared.push_back(("-arch=" + ctx->clangTargetInfo->getTriple().getArchName().str()).c_str());
        }
        auto sysrootStr = String("-syslibroot=\"").append(cfg->has_sysroot() ? cfg->get_sysroot() : "").append("\"");
        if (cfg->has_sysroot()) {
          allArgs.push_back(sysrootStr.c_str());
          allArgsShared.push_back(sysrootStr.c_str());
        } else if (ctx->clangTargetInfo->getTriple().isMacOSX()) {
          if (hostTriplet.isTargetMachineMac() && fs::exists(MACOS_DEFAULT_SDK_PATH)) {
            sysrootStr = String("-syslibroot=").append("\"" MACOS_DEFAULT_SDK_PATH "\"");
            allArgs.push_back(sysrootStr.c_str());
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
                           " failed with return code " + ctx->color(std::to_string(result.retCode)) +
                           ". The linker's error output is: " + stdErrStr,
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
                           " failed with return code " + ctx->color(std::to_string(resultShared.retCode)) +
                           ". The linker's error output is: " + stdErrStr,
                       None);
          }
        }
      } else {
        if (cfg->has_linker_path()) {
          auto cmd = String(cfg->get_linker_path()).append(" -o ").append(outPath).append(" ");
          for (auto& obj : objectFiles) {
            cmd.append(obj).append(" ");
          }
          redi::ipstream proc(cmd, redi::pstreams::pstderr);
          String         errMessage;
          String         line;
          while (std::getline(proc.err(), line)) {
            errMessage += line += "\n";
          }
          while (!proc.rdbuf()->exited()) {
          }
          if (proc.rdbuf()->status()) {
            ctx->Error("Building library for module " + ctx->color(name.value) + "" + ctx->color(filePath.string()) +
                           " failed\n" + errMessage,
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
            llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_join", llvmModule);
        linkPthread = true;
      }
      break;
    }
    case NativeUnit::pthreadExit: {
      if (!llvmModule->getFunction("pthread_exit")) {
        llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx),
                                                       {llvm::Type::getInt8Ty(llCtx)->getPointerTo()}, false),
                               llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, "pthread_exit", llvmModule);
        linkPthread = true;
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
