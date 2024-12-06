#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "../utils/qat_region.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./emit_phase.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./types/float.hpp"
#include "./types/mix.hpp"
#include "entity_overview.hpp"
#include "link_names.hpp"
#include "lld/Common/Driver.h"
#include "meta_info.hpp"
#include "types/definition.hpp"
#include "value.hpp"

#include <llvm/IR/LLVMContext.h>
#include <set>

LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(wasm)

namespace qat {
class QatSitter;
}

namespace qat::ast {
class Node;
class IsEntity;
class Lib;
class ModInfo;
class BringPaths;
class BinaryExpression;
class BringBitwidths;
class BringEntities;
class MethodCall;
class MemberAccess;
} // namespace qat::ast

namespace qat::ir {

class Ctx;
class PrerunFunction;
class GenericStructType;

enum class ModuleType { lib, file, folder };

enum class InternalDependency {
  printf,
  malloc,
  free,
  realloc,
  pthreadCreate,
  pthreadJoin,
  pthreadExit,
  pthreadAttrInit,
  windowsExitThread,
  exitProgram,
  panicHandler,
};

useit inline String internal_dependency_to_string(InternalDependency unit) {
  switch (unit) {
    case InternalDependency::printf:
      return "printf";
    case InternalDependency::malloc:
      return "malloc";
    case InternalDependency::free:
      return "free";
    case InternalDependency::realloc:
      return "realloc";
    case InternalDependency::pthreadCreate:
      return "pthread_create";
    case InternalDependency::pthreadJoin:
      return "pthread_join";
    case InternalDependency::pthreadExit:
      return "pthread_exit";
    case InternalDependency::pthreadAttrInit:
      return "pthread_attr_init";
    case InternalDependency::windowsExitThread:
      return "__imp_ExitThread";
    case InternalDependency::exitProgram:
      return "exit";
    case InternalDependency::panicHandler:
      return "panicHandler";
  }
}

useit inline Maybe<InternalDependency> internal_dependency_from_string(String value) {
  if (value == "printf") {
    return InternalDependency::printf;
  } else if (value == "malloc") {
    return InternalDependency::malloc;
  } else if (value == "free") {
    return InternalDependency::free;
  } else if (value == "realloc") {
    return InternalDependency::realloc;
  } else if (value == "pthread_create") {
    return InternalDependency::pthreadCreate;
  } else if (value == "pthread_join") {
    return InternalDependency::pthreadJoin;
  } else if (value == "pthread_exit") {
    return InternalDependency::pthreadExit;
  } else if (value == "pthread_attr_init") {
    return InternalDependency::pthreadAttrInit;
  } else if (value == "__imp_ExitThread") {
    return InternalDependency::windowsExitThread;
  } else if (value == "exit") {
    return InternalDependency::exitProgram;
  } else if (value == "panicHandler") {
    return InternalDependency::panicHandler;
  }
  return None;
}

enum class IntrinsicID { varArgStart, varArgEnd, varArgCopy, vectorScale };

enum class LibToLinkType {
  namedLib,
  libPath,
  staticAndSharedPaths,
  nameWithLookupPath,
};

class LibToLink {

  LibToLink(LibToLinkType _type, FileRange _fileRange) : type(_type), fileRange(_fileRange) {}

public:
  Maybe<Identifier>              name;
  Maybe<Pair<String, FileRange>> path;
  Maybe<Pair<String, FileRange>> sharedPath;
  LibToLinkType                  type;
  FileRange                      fileRange;

  useit static LibToLink fromName(Identifier _name, FileRange _fileRange) {
    LibToLink result(LibToLinkType::namedLib, _fileRange);
    result.name = _name;
    return result;
  }

  useit static LibToLink fromPath(Pair<String, FileRange> _path, FileRange _fileRange) {
    LibToLink result(LibToLinkType::libPath, _fileRange);
    result.path = _path;
    return result;
  }

  useit static LibToLink fromStaticAndShared(Pair<String, FileRange> _staticPath, Pair<String, FileRange> _sharedPath,
                                             FileRange _fileRange) {
    LibToLink result(LibToLinkType::staticAndSharedPaths, _fileRange);
    result.path       = _staticPath;
    result.sharedPath = _sharedPath;
    return result;
  }

  useit static LibToLink fromNameWithPath(Identifier _name, Pair<String, FileRange> _path, FileRange _fileRange) {
    LibToLink result(LibToLinkType::nameWithLookupPath, _fileRange);
    result.name = _name;
    result.path = _path;
    return result;
  }

  useit bool isName() { return type == LibToLinkType::namedLib; }
  useit bool isLibPath() { return type == LibToLinkType::libPath; }
  useit bool isStaticAndSharedPaths() { return type == LibToLinkType::staticAndSharedPaths; }
  useit bool isNameWithLookupPath() { return type == LibToLinkType::nameWithLookupPath; }

  useit bool operator==(LibToLink const& other) {
    if (type == other.type) {
      switch (type) {
        case LibToLinkType::namedLib: {
          return name.value().value == other.name.value().value;
        }
        case LibToLinkType::libPath: {
          return fs::absolute(fs::path(path->first).is_relative() ? (fileRange.file / path->first)
                                                                  : fs::path(path->first)) ==
                 fs::absolute(fs::path(other.path->first).is_relative() ? (fileRange.file / other.path->first)
                                                                        : fs::path(other.path->first));
        }
        case LibToLinkType::staticAndSharedPaths: {
          return (fs::absolute(fs::path(path->first).is_relative() ? (fileRange.file / path->first)
                                                                   : fs::path(path->first)) ==
                  fs::absolute(fs::path(other.path->first).is_relative() ? (fileRange.file / other.path->first)
                                                                         : fs::path(other.path->first))) &&
                 (fs::absolute(fs::path(sharedPath->first).is_relative() ? (fileRange.file / sharedPath->first)
                                                                         : fs::path(sharedPath->first)) ==
                  fs::absolute(fs::path(other.sharedPath->first).is_relative()
                                   ? (fileRange.file / other.sharedPath->first)
                                   : fs::path(other.sharedPath->first)));
        }
        case LibToLinkType::nameWithLookupPath: {
          return (name.value().value == other.name.value().value) &&
                 (fs::absolute(fs::path(path->first).is_relative() ? (fileRange.file / path->first)
                                                                   : fs::path(path->first)) ==
                  fs::absolute(fs::path(other.path->first).is_relative() ? (fileRange.file / other.path->first)
                                                                         : fs::path(other.path->first)));
        }
      }
    } else {
      return false;
    }
  }
};

enum class EntityType {
  structType,
  choiceType,
  mixType,
  function,
  genericFunction,
  genericStructType,
  genericTypeDef,
  typeDefinition,
  region,
  global,
  prerunGlobal,
  opaque,
  bringEntity,
  defaultDoneSkill,
};

inline String entity_type_to_string(EntityType ty) {
  switch (ty) {
    case EntityType::structType:
      return "struct type";
    case EntityType::choiceType:
      return "choice type";
    case EntityType::mixType:
      return "mix type";
    case EntityType::function:
      return "function";
    case EntityType::genericFunction:
      return "generic function";
    case EntityType::genericStructType:
      return "generic struct type";
    case EntityType::genericTypeDef:
      return "generic type definition";
    case EntityType::typeDefinition:
      return "type definition";
    case EntityType::region:
      return "region";
    case EntityType::global:
      return "global";
    case EntityType::prerunGlobal:
      return "prerun global";
    case EntityType::opaque:
      return "opaque type";
    case EntityType::bringEntity:
      return "brought entity";
    case EntityType::defaultDoneSkill:
      return "type extension";
  }
}

enum class EntityStatus { none, partial, complete, childrenPartial };
enum class DependType { partial, complete, childrenPartial };
enum class EntityChildType { staticFn, method, variation, valued };

inline String entity_child_type_to_string(EntityChildType type) {
  switch (type) {
    case EntityChildType::staticFn:
      return "static method";
    case EntityChildType::method:
      return "method";
    case EntityChildType::variation:
      return "variation method";
    case EntityChildType::valued:
      return "value method";
  }
}

struct EntityState;
struct EntityDependency {
  EntityState* entity;
  DependType   type;
  EmitPhase    phase;
};

struct EntityState {
  Maybe<Identifier>     name;
  EntityType            type;
  EntityStatus          status;
  ast::IsEntity*        astNode = nullptr;
  EmitPhase             maxPhase;
  Vec<EntityDependency> dependencies;

  std::set<Pair<EntityChildType, String>> children;

  Maybe<EmitPhase> phaseToCompletion;
  Maybe<EmitPhase> phaseToPartial;
  bool             supportsChildren = false;
  Maybe<EmitPhase> phaseToChildrenPartial;

  Maybe<EmitPhase> currentPhase;

  usize iterations = 0;

  EntityState(Maybe<Identifier> _name, EntityType _type, EntityStatus _status, ast::IsEntity* _astEntity,
              EmitPhase _maxPhase)
      : name(_name), type(_type), status(_status), astNode(_astEntity), maxPhase(_maxPhase) {}

  void addDependency(EntityDependency dep) {
    if (this == dep.entity) {
      return;
    }
    bool alreadyPresent = false;
    for (auto& it : dependencies) {
      if (it.entity == dep.entity && it.phase == dep.phase && it.type == dep.type) {
        alreadyPresent = true;
        break;
      }
    }
    if (!alreadyPresent) {
      dependencies.push_back(dep);
    }
  }

  void updateStatus(EntityStatus _status) { status = _status; }

  useit bool has_child(String const& child) {
    for (auto& ch : children) {
      if (ch.second == child) {
        return true;
      }
    }
    return false;
  }

  useit Pair<EntityChildType, String> get_child(String const& name) {
    for (auto& ch : children) {
      if (ch.second == name) {
        return ch;
      }
    }
  }

  void add_child(Pair<EntityChildType, String> child) { children.insert(child); }

  useit bool are_all_phases_complete() { return currentPhase.has_value() && (currentPhase.value() == maxPhase); }

  useit bool is_ready_for_next_phase() {
    auto nextPhase = get_next_phase(currentPhase);
    if (nextPhase.has_value()) {
      bool isAllDepsComplete = true;
      for (auto& dep : dependencies) {
        if (dep.phase == nextPhase.value()) {
          if (dep.type == DependType::partial) {
            if (dep.entity->status == EntityStatus::none) {
              isAllDepsComplete = false;
              break;
            }
          } else if (dep.type == DependType::complete) {
            if (dep.entity->status < EntityStatus::complete) {
              isAllDepsComplete = false;
              break;
            }
          } else if (dep.type == DependType::childrenPartial) {
            if (dep.entity->supportsChildren ? (dep.entity->status < EntityStatus::childrenPartial)
                                             : (dep.entity->status < EntityStatus::complete)) {
              isAllDepsComplete = false;
              break;
            }
          }
        }
      }
      return isAllDepsComplete;
    }
    return false;
  }

  void do_next_phase(ir::Mod* mod, ir::Ctx* irCtx);
};

class Mod final : public Uniq, public EntityOverview {
  friend class Region;
  friend class OpaqueType;
  friend class StructType;
  friend class MixType;
  friend class ChoiceType;
  friend class DefinitionType;
  friend class GlobalEntity;
  friend class PrerunGlobal;
  friend class ast::Lib;
  friend class ast::ModInfo;
  friend class ast::BringPaths;
  friend class GenericFunction;
  friend class GenericStructType;
  friend class GenericDefinitionType;
  friend class Function;
  friend class ast::BinaryExpression;
  friend class ast::BringBitwidths;
  friend class ast::BringEntities;
  friend class qat::QatSitter;
  friend class ast::MethodCall;
  friend class ast::MemberAccess;

public:
  Mod(Identifier _name, fs::path _filePath, fs::path _basePath, ModuleType _type, const VisibilityInfo& _visibility,
      ir::Ctx* irCtx);

  static Vec<Mod*>     allModules;
  static Vec<fs::path> usableNativeLibPaths;
  static Maybe<String> usableClangPath;

  static Maybe<fs::path> windowsMSVCLibPath;
  static Maybe<fs::path> windowsATLMFCLibPath;
  static Maybe<fs::path> windowsUCRTLibPath;
  static Maybe<fs::path> windowsUMLibPath;

  static void clear_all();

  useit static bool has_file_module(const fs::path& fPath);
  useit static bool has_folder_module(const fs::path& fPath);

  useit static Mod* get_file_module(const fs::path& fPath);
  useit static Mod* get_folder_module(const fs::path& fPath);

private:
  Identifier        name;
  ModuleType        moduleType;
  bool              rootLib = false;
  Maybe<MetaInfo>   metaInfo;
  Deque<LibToLink>  nativeLibsToLink;
  fs::path          filePath;
  fs::path          basePath;
  VisibilityInfo    visibility;
  Mod*              parent = nullptr;
  Mod*              active = nullptr;
  std::set<Mod*>    dependencies;
  Vec<Mod*>         submodules;
  Vec<Brought<Mod>> broughtModules;

  Deque<OpaqueType*>       opaqueTypes;
  Vec<Brought<OpaqueType>> broughtOpaqueTypes;
  Vec<Brought<OpaqueType>> broughtGenericOpaqueTypes;

  Vec<StructType*>         coreTypes;
  Vec<Brought<StructType>> broughtCoreTypes;

  Vec<ChoiceType*>         choiceTypes;
  Vec<Brought<ChoiceType>> broughtChoiceTypes;

  Vec<MixType*>         mixTypes;
  Vec<Brought<MixType>> broughtMixTypes;

  Vec<DefinitionType*>         typeDefs;
  Vec<Brought<DefinitionType>> broughtTypeDefs;

  Vec<Function*>         functions;
  Vec<Brought<Function>> broughtFunctions;

  Vec<PrerunFunction*>         prerunFunctions;
  Vec<Brought<PrerunFunction>> broughtPrerunFunctions;

  Vec<GenericFunction*>         genericFunctions;
  Vec<Brought<GenericFunction>> broughtGenericFunctions;

  Vec<GenericStructType*>         genericCoreTypes;
  Vec<Brought<GenericStructType>> broughtGenericCoreTypes;

  Vec<GenericDefinitionType*>         genericTypeDefinitions;
  Vec<Brought<GenericDefinitionType>> broughtGenericTypeDefinitions;

  Vec<GlobalEntity*>         globalEntities;
  Vec<Brought<GlobalEntity>> broughtGlobalEntities;

  Vec<PrerunGlobal*>         prerunGlobals;
  Vec<Brought<PrerunGlobal>> broughtPrerunGlobals;

  Vec<Region*>         regions;
  Vec<Brought<Region>> broughtRegions;

  Vec<EntityState*> entityEntries;

  Function* moduleInitialiser   = nullptr;
  Function* moduleDeinitialiser = nullptr;
  u64       nonConstantGlobals  = 0;

  std::set<u64> integerBitwidths;
  std::set<u64> unsignedBitwidths;

  std::set<FloatTypeKind> floatKinds;

  Vec<Pair<Mod*, FileRange>> fsBroughtMentions;

  Vec<ast::Node*> nodes;
  bool            hasMain = false;
  fs::path        llPath;
  Maybe<fs::path> objectFilePath;

  mutable llvm::Module* llvmModule;
  mutable Maybe<String> moduleForeignID;

  mutable bool linkPthread                  = false;
  mutable bool hasCreatedModules            = false;
  mutable bool hasHandledFilesystemBrings   = false;
  mutable bool hasCreatedEntities           = false;
  mutable bool hasUpdatedEntityDependencies = false;
  mutable bool isOverviewOutputted          = false;

  bool isCompiledToObject = false;
  bool isBundled          = false;

  void addMember(Mod* mod);

  void addNamedSubmodule(const Identifier& name, const String& _filename, ModuleType type,
                         const VisibilityInfo& visib_info, ir::Ctx* irCtx);
  void closeSubmodule();

  useit bool should_be_named() const;

  static std::map<InternalDependency, Function*> providedFunctions;

public:
  ~Mod();

  useit static Mod* create(const Identifier& name, const fs::path& filepath, const fs::path& basePath, ModuleType type,
                           const VisibilityInfo& visib_info, ir::Ctx* irCtx);
  useit static Mod* create_submodule(Mod* parent, fs::path _filepath, fs::path basePath, Identifier name,
                                     ModuleType type, const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx);
  useit static Mod* create_file_mod(Mod* parent, fs::path _filepath, fs::path basePath, Identifier name,
                                    Vec<ast::Node*>, VisibilityInfo visibilityInfo, ir::Ctx* irCtx);
  useit static Mod* create_root_lib(Mod* parent, fs::path _filePath, fs::path basePath, Identifier name,
                                    Vec<ast::Node*> nodes, const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

  useit static bool has_provided_function(InternalDependency unit) { return providedFunctions.contains(unit); }
  static void       add_provided_function(InternalDependency unit, Function* fnVal) { providedFunctions[unit] = fnVal; }
  useit static Function* get_provided_function(InternalDependency unit) { return providedFunctions[unit]; }

  static bool triple_is_equivalent(llvm::Triple const& first, llvm::Triple const& second);

  static Vec<Function*> collect_mod_initialisers();

  useit bool has_entity_with_name(String const& name) {
    for (auto ent : entityEntries) {
      if (ent->name.has_value() && ent->name->value == name) {
        return true;
      }
    }
    for (auto sub : submodules) {
      if (!sub->should_be_named()) {
        if (sub->has_entity_with_name(name)) {
          return true;
        }
      }
    }
    for (auto bMod : broughtModules) {
      if (!bMod.is_named() && !bMod.get()->should_be_named()) {
        if (bMod.get()->has_entity_with_name(name)) {
          return true;
        }
      }
    }
    return false;
  }

  useit EntityState* add_entity(Maybe<Identifier> name, EntityType type, ast::IsEntity* node, EmitPhase maxPhase) {
    entityEntries.push_back(new EntityState(name, type, EntityStatus::none, node, maxPhase));
    return entityEntries.back();
  }

  useit EntityState* get_entity(String const& name) {
    for (auto ent : entityEntries) {
      if (ent->name.has_value() && ent->name->value == name) {
        return ent;
      }
    }
    for (auto sub : submodules) {
      if (!sub->should_be_named()) {
        if (sub->has_entity_with_name(name)) {
          return sub->get_entity(name);
        }
      }
    }
    for (auto bMod : broughtModules) {
      if (!bMod.is_named() && !bMod.get()->should_be_named()) {
        if (bMod.get()->has_entity_with_name(name)) {
          return bMod.get()->get_entity(name);
        }
      }
    }
    return nullptr;
  }

  void entity_name_check(ir::Ctx* irCtx, Identifier name, ir::EntityType entTy);

  useit ModuleType get_mod_type() const;
  useit String     get_full_name() const;
  useit String     get_writable_name() const;
  useit String     get_name() const;
  useit Identifier get_identifier() const;
  useit String     get_fullname_with_child(const String& name) const;
  useit Mod*       get_active();
  useit Mod*       get_parent_file();

  useit String get_file_path() const { return filePath.string(); }

  void            set_file_range(FileRange fileRange);
  useit FileRange get_file_range() const;

  useit Function* get_mod_initialiser(ir::Ctx* irCtx);
  useit bool      should_call_initialiser() const;
  void            add_non_const_global_counter();

  useit LinkNames get_link_names() const;

  useit bool is_parent_mod_of(Mod* other) const;
  useit bool has_parent_lib() const;
  useit Mod* get_closest_parent_lib();

  useit bool has_meta_info_key(String key) const;
  useit bool has_meta_info_key_in_parent(String key) const;
  useit bool is_in_foreign_mod_of_type(String id) const;
  useit Maybe<ir::PrerunValue*> get_meta_info_for_key(String key) const;
  useit Maybe<ir::PrerunValue*> get_meta_info_from_parent(String key) const;
  useit Maybe<String> get_relevant_foreign_id() const;

  useit bool has_nth_parent(u32 n) const;
  useit Mod* get_nth_parent(u32 n);

  useit const VisibilityInfo& get_visibility() const;

  useit Function* create_function(const Identifier& name, bool isInline, Type* returnType, Vec<Argument> args,
                                  const FileRange& fileRange, const VisibilityInfo& visibility,
                                  Maybe<llvm::GlobalValue::LinkageTypes> linkage, ir::Ctx* irCtx);

  useit bool is_submodule() const { return parent != nullptr; }
  useit bool has_submodules() const { return !submodules.empty(); }

  void add_dependency(ir::Mod* dep);

  useit bool has_integer_bitwidth(u64 bits) const {
    return (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) ||
           integerBitwidths.contains(bits);
  }
  useit bool has_unsigned_bitwidth(u64 bits) const {
    return (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) ||
           unsignedBitwidths.contains(bits);
  }
  useit bool has_float_kind(FloatTypeKind kind) const {
    return (kind == FloatTypeKind::_32 || kind == FloatTypeKind::_64) || floatKinds.contains(kind);
  }

  void add_integer_bitwidth(u64 bits) { integerBitwidths.insert(bits); }

  void add_unsigned_bitwidth(u64 bits) { unsignedBitwidths.insert(bits); }

  void add_float_kind(FloatTypeKind kind) { floatKinds.insert(kind); }

  useit bool has_main_function() const { return hasMain; }
  void       set_has_main_function() { hasMain = true; }

  useit std::set<String> getAllObjectPaths() const;
  useit std::set<String> getAllLinkableLibs() const;

  void                              add_fs_bring_mention(ir::Mod* otherMod, const FileRange& fileRange);
  Vec<Pair<Mod*, FileRange>> const& get_fs_bring_mentions() const;

  void update_overview() final;
  void output_all_overview(Vec<JsonValue>& modulesJson, Vec<JsonValue>& functionsJson,
                           Vec<JsonValue>& prerunFunctionJSON, Vec<JsonValue>& genericFunctionsJson,
                           Vec<JsonValue>& genericCoreTypesJson, Vec<JsonValue>& coreTypesJson,
                           Vec<JsonValue>& mixTypesJson, Vec<JsonValue>& regionJson, Vec<JsonValue>& choiceJson,
                           Vec<JsonValue>& defsJson);

  // LIB

  useit bool has_lib(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_lib(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_lib_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit Mod*               get_lib(const String& name, const AccessInfo& reqInfo);

  void open_lib_for_creation(const Identifier& name, const String& filename, const VisibilityInfo& visib_info,
                             ir::Ctx* irCtx);
  void close_lib_after_creation();

  // FUNCTION

  useit bool      has_function(const String& name, AccessInfo reqInfo) const;
  useit bool      has_brought_function(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Function* get_function(const String& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> has_function_in_imports(const String& name, const AccessInfo& reqInfo) const;

  // PRERUN FUNCTION

  useit bool            has_prerun_function(String const& name, AccessInfo reqInfo) const;
  useit bool            has_brought_prerun_function(String const& name, Maybe<AccessInfo> reqInfo) const;
  useit PrerunFunction* get_prerun_function(String const& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> has_prerun_function_in_imports(String const& name, AccessInfo const& reqInfo) const;

  // GENERIC FUNCTIONS

  useit bool             has_generic_function(const String& name, AccessInfo reqInfo) const;
  useit bool             has_brought_generic_function(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit GenericFunction* get_generic_function(const String& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> has_generic_function_in_imports(const String& name, const AccessInfo& reqInfo) const;

  // REGION

  useit bool has_region(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_region(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_region_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit Region*            get_region(const String& name, const AccessInfo& reqInfo) const;

  // OPAQUE TYPES

  useit bool has_opaque_type(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_opaque_type(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_opaque_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit OpaqueType*        get_opaque_type(const String& name, const AccessInfo& reqInfo) const;

  // CORE TYPE

  useit bool has_struct_type(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit StructType*        get_struct_type(const String& name, const AccessInfo& reqInfo) const;

  // MIX TYPE

  useit bool has_mix_type(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_mix_type(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_mix_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit MixType*           get_mix_type(const String& name, const AccessInfo& reqInfo) const;

  // CHOICE TYPE

  useit bool has_choice_type(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_choice_type(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_choice_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit ChoiceType*        get_choice_type(const String& name, const AccessInfo& reqInfo) const;

  // GENERIC STRUCT TYPES

  useit bool has_generic_struct_type(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_generic_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_generic_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit GenericStructType* get_generic_struct_type(const String& name, const AccessInfo& reqInfo);

  // GENERIC TYPEDEFS

  useit bool has_generic_type_def(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_generic_type_def(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String>     has_generic_type_def_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit GenericDefinitionType* get_generic_type_def(const String& name, const AccessInfo& reqInfo);

  // TYPEDEFS

  useit bool has_type_definition(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_type_definition(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_type_definition_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit DefinitionType*    get_type_def(const String& name, const AccessInfo& reqInfo) const;

  // GLOBAL

  useit bool has_global(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_global(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_global_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit GlobalEntity*      get_global(const String& name, const AccessInfo& reqInfo) const;

  // PRERUN GLOBAL

  useit bool has_prerun_global(const String& name, AccessInfo reqInfo) const;
  useit bool has_brought_prerun_global(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> has_prerun_global_in_imports(const String& name, const AccessInfo& reqInfo) const;
  useit PrerunGlobal*      get_prerun_global(const String& name, const AccessInfo& reqInfo) const;

  // IMPORT

  useit bool has_brought_mod(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Mod* get_brought_mod(const String& name, const AccessInfo& reqInfo) const;
  useit Pair<bool, String> has_brought_mod_in_imports(const String& name, const AccessInfo& reqInfo) const;

  // BRING ENTITIES

  void bring_module(Mod* other, const VisibilityInfo& _visib, Maybe<Identifier> bName = None);
  void bring_struct_type(StructType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_opaque_type(OpaqueType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_mix_type(MixType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_choice_type(ChoiceType* chTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_type_definition(DefinitionType* dTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_function(Function* fn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_prerun_function(PrerunFunction* preFn, VisibilityInfo const& visib, Maybe<Identifier> bName = None);
  void bring_region(Region* reg, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_global(GlobalEntity* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_prerun_global(PrerunGlobal* preGlobal, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_generic_struct_type(GenericStructType* gCTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_generic_function(GenericFunction* gFn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bring_generic_type_definition(GenericDefinitionType* gTDef, VisibilityInfo const& visib,
                                     Maybe<Identifier> bName = None);

  useit fs::path get_resolved_output_path(const String& extension, Ctx* irCtx);
  useit llvm::Module* get_llvm_module() const;
  useit Maybe<fs::path> find_static_library_path(String libName) const;

  useit bool find_clang_path(Ctx* irCtx);
  useit bool find_windows_sdk_paths(Ctx* irCtx);
  useit bool find_windows_toolchain_libs(Ctx* irCtx, bool findMSVCLibPath, bool findATLMFCLibPath, bool findUCRTLibPath,
                                         bool findUMLibPath);

  static void find_native_library_paths();

  void node_create_modules(Ctx* irCtx);
  void node_handle_fs_brings(Ctx* irCtx);
  void node_create_entities(Ctx* irCtx);
  void node_update_dependencies(Ctx* irCtx);

  void setup_llvm_file(Ctx* irCtx);
  void compile_to_object(Ctx* irCtx);
  void handle_native_libs(Ctx* irCtx);
  void bundle_modules(Ctx* irCtx);

  void export_json_from_ast(Ctx* irCtx);

  /// This returns the name of the linked function or symbol
  /// Even known units like printf can have a different name for the underlying function, especially in freehosting
  /// environments
  useit String link_internal_dependency(InternalDependency nval, Ctx* irCtx, FileRange fileRange);

  useit llvm::Function* link_intrinsic(IntrinsicID intr);

  Json to_json() const;
};

} // namespace qat::ir

#endif
