#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../show.hpp"
#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "../utils/qat_region.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./emit_phase.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./link_names.hpp"
#include "./meta_info.hpp"
#include "./types/float.hpp"
#include "./value.hpp"

#include <helpers/deque.hpp>
#include <helpers/files.hpp>
#include <helpers/hashmap.hpp>
#include <helpers/hashset.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>
#include <lld/Common/Driver.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Passes/PassBuilder.h>

LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(wasm)

namespace llvm {
class TargetMachine;
};

namespace qat {
class QatSitter;
}

namespace qat::ast {
class Node;
class IsEntity;
class Lib;
class ModInfo;
class ImportPaths;
class BinaryExpression;
class ImportBitwidths;
class ImportEntities;
class MethodCall;
class MemberAccess;
class Entity;
} // namespace qat::ast

namespace qat::ir {

class Ctx;
class GenericDefinitionType;
class GenericToggleType;
class GenericSkill;

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

inline String internal_dependency_to_string(InternalDependency unit) {
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

inline Maybe<InternalDependency> internal_dependency_from_string(String value) {
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

	LibToLink(LibToLinkType _type, FileRangePtr _fileRange) : type(_type), fileRange(_fileRange) {}

  public:
	Maybe<Identifier>                 name;
	Maybe<Pair<String, FileRangePtr>> path;
	Maybe<Pair<String, FileRangePtr>> sharedPath;
	LibToLinkType                     type;
	FileRangePtr                      fileRange;

	static LibToLink fromName(Identifier _name, FileRangePtr _fileRange) {
		LibToLink result(LibToLinkType::namedLib, _fileRange);
		result.name = _name;
		return result;
	}

	static LibToLink fromPath(Pair<String, FileRangePtr> _path, FileRangePtr _fileRange) {
		LibToLink result(LibToLinkType::libPath, _fileRange);
		result.path = _path;
		return result;
	}

	static LibToLink fromStaticAndShared(Pair<String, FileRangePtr> _staticPath, Pair<String, FileRangePtr> _sharedPath,
	                                     FileRangePtr _fileRange) {
		LibToLink result(LibToLinkType::staticAndSharedPaths, _fileRange);
		result.path       = _staticPath;
		result.sharedPath = _sharedPath;
		return result;
	}

	static LibToLink fromNameWithPath(Identifier _name, Pair<String, FileRangePtr> _path, FileRangePtr _fileRange) {
		LibToLink result(LibToLinkType::nameWithLookupPath, _fileRange);
		result.name = _name;
		result.path = _path;
		return result;
	}

	bool isName() { return type == LibToLinkType::namedLib; }

	bool isLibPath() { return type == LibToLinkType::libPath; }

	bool isStaticAndSharedPaths() { return type == LibToLinkType::staticAndSharedPaths; }

	bool isNameWithLookupPath() { return type == LibToLinkType::nameWithLookupPath; }

	bool operator==(LibToLink const& other) {
		if (type == other.type) {
			switch (type) {
				case LibToLinkType::namedLib: {
					return name.value().value == other.name.value().value;
				}
				case LibToLinkType::libPath: {
					const auto filePath = FilePath(*fileRange->file);
					return fs::absolute(FilePath(path->first).is_relative() ? (filePath / path->first)
					                                                        : FilePath(path->first)) ==
					       fs::absolute(FilePath(other.path->first).is_relative() ? (filePath / other.path->first)
					                                                              : FilePath(other.path->first));
				}
				case LibToLinkType::staticAndSharedPaths: {
					const auto filePath = FilePath(*fileRange->file);
					return (fs::absolute(FilePath(path->first).is_relative() ? (filePath / path->first)
					                                                         : FilePath(path->first)) ==
					        fs::absolute(FilePath(other.path->first).is_relative() ? (filePath / other.path->first)
					                                                               : FilePath(other.path->first))) &&
					       (fs::absolute(FilePath(sharedPath->first).is_relative() ? (filePath / sharedPath->first)
					                                                               : FilePath(sharedPath->first)) ==
					        fs::absolute(FilePath(other.sharedPath->first).is_relative()
					                         ? (filePath / other.sharedPath->first)
					                         : FilePath(other.sharedPath->first)));
				}
				case LibToLinkType::nameWithLookupPath: {
					const auto filePath = FilePath(*fileRange->file);
					return (name.value().value == other.name.value().value) &&
					       (fs::absolute(FilePath(path->first).is_relative() ? (filePath / path->first)
					                                                         : FilePath(path->first)) ==
					        fs::absolute(FilePath(other.path->first).is_relative() ? (filePath / other.path->first)
					                                                               : FilePath(other.path->first)));
				}
			}
		} else {
			return false;
		}
	}
};

enum class EntityType {
	assemblyBlock,
	structType,
	choiceType,
	flagType,
	mixType,
	toggleType,
	function,
	prerunFunction,
	genericFunction,
	genericStructType,
	genericMixType,
	genericToggleType,
	genericTypeDef,
	typeDefinition,
	region,
	global,
	prerunGlobal,
	opaque,
	importEntity,
	defaultDoneSkill,
	doneSkill,
	skill,
	genericSkill,
};

inline String entity_type_to_string(EntityType ty) {
	switch (ty) {
		case EntityType::assemblyBlock:
			return "assembly block";
		case EntityType::structType:
			return "struct type";
		case EntityType::choiceType:
			return "choice type";
		case EntityType::flagType:
			return "flag type";
		case EntityType::mixType:
			return "mix type";
		case EntityType::toggleType:
			return "toggle type";
		case EntityType::function:
			return "function";
		case EntityType::prerunFunction:
			return "prerun function";
		case EntityType::genericFunction:
			return "generic function";
		case EntityType::genericStructType:
			return "generic struct type";
		case EntityType::genericMixType:
			return "generic mix type";
		case EntityType::genericToggleType:
			return "generic toggle type";
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
		case EntityType::importEntity:
			return "brought entity";
		case EntityType::defaultDoneSkill:
			return "type extension";
		case EntityType::doneSkill:
			return "skill implementation";
		case EntityType::skill:
			return "skill";
		case EntityType::genericSkill:
			return "generic skill";
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

	static EntityState* create(Maybe<Identifier> name, EntityType type, EntityStatus status, ast::IsEntity* astEntity,
	                           EmitPhase maxPhase) {
		return std::construct_at(OwnNormal(EntityState), std::move(name), type, status, astEntity, maxPhase);
	}

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
		if (not alreadyPresent) {
			dependencies.push_back(dep);
		}
	}

	void updateStatus(EntityStatus _status) { status = _status; }

	bool has_child(String const& child) const {
		for (auto& ch : children) {
			if (ch.second == child) {
				return true;
			}
		}
		return false;
	}

	Pair<EntityChildType, String> get_child(String const& name) {
		for (auto& ch : children) {
			if (ch.second == name) {
				return ch;
			}
		}
		std::unreachable();
	}

	void add_child(Pair<EntityChildType, String> child) { children.insert(child); }

	bool are_all_phases_complete() const { return currentPhase.has_value() && (currentPhase.value() == maxPhase); }

	void complete_manually() {
		status       = EntityStatus::complete;
		currentPhase = maxPhase;
	}

	bool is_ready_for_next_phase() const {
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

class PrerunFunction;
class GenericStructType;
class Skill;
struct ModTypeInfo;

class Mod final : public Uniq, public Mentionable {
	friend class Region;
	friend class OpaqueType;
	friend class StructType;
	friend class MixType;
	friend class ToggleType;
	friend class ChoiceType;
	friend class FlagType;
	friend class DefinitionType;
	friend class GlobalEntity;
	friend class PrerunGlobal;
	friend class PrerunFunction;
	friend class ast::Lib;
	friend class ast::ModInfo;
	friend class ast::ImportPaths;
	friend class GenericFunction;
	friend class GenericStructType;
	friend class GenericDefinitionType;
	friend class GenericToggleType;
	friend class Skill;
	friend class GenericSkill;
	friend class Function;
	friend class ast::BinaryExpression;
	friend class ast::ImportBitwidths;
	friend class ast::ImportEntities;
	friend class qat::QatSitter;
	friend class ast::MethodCall;
	friend class ast::MemberAccess;
	friend class ast::Entity;
	friend struct ModTypeInfo;
	friend struct TypeInfo;

  public:
	Mod(Identifier _name, FilePath _filePath, FilePath _basePath, ModuleType _type, const VisibilityInfo& _visibility,
	    ir::Ctx* irCtx);

	static Vec<Mod*>     allModules;
	static Vec<FilePath> usableNativeLibPaths;
	static Maybe<String> usableClangPath;

	static Maybe<FilePath> windowsMSVCLibPath;
	static Maybe<FilePath> windowsATLMFCLibPath;
	static Maybe<FilePath> windowsUCRTLibPath;
	static Maybe<FilePath> windowsUMLibPath;

	static void clear_all();

	static bool has_file_module(const FilePath& fPath);
	static bool has_folder_module(const FilePath& fPath);

	static Mod* get_file_module(const FilePath& fPath);
	static Mod* get_folder_module(const FilePath& fPath);

  private:
	Identifier        name;
	ModuleType        moduleType;
	bool              rootLib = false;
	Maybe<MetaInfo>   metaInfo;
	Deque<LibToLink>  nativeLibsToLink;
	FilePath          filePath;
	FilePath          basePath;
	VisibilityInfo    visibility;
	Mod*              parent = nullptr;
	Mod*              active = nullptr;
	std::set<Mod*>    dependencies;
	Vec<Mod*>         submodules;
	Vec<Brought<Mod>> importedModules;

	ModTypeInfo* typeInfoDetail = nullptr;

	Deque<OpaqueType*>       opaqueTypes;
	Vec<Brought<OpaqueType>> broughtOpaqueTypes;
	Vec<Brought<OpaqueType>> broughtGenericOpaqueTypes;

	Vec<StructType*>         structTypes;
	Vec<Brought<StructType>> broughtStructTypes;

	Vec<ChoiceType*>         choiceTypes;
	Vec<Brought<ChoiceType>> broughtChoiceTypes;

	Vec<FlagType*>         flagTypes;
	Vec<Brought<FlagType>> broughtFlagTypes;

	Vec<MixType*>         mixTypes;
	Vec<Brought<MixType>> broughtMixTypes;

	Vec<ToggleType*>         toggleTypes;
	Vec<Brought<ToggleType>> broughtToggleTypes;

	Vec<DefinitionType*>         typeDefs;
	Vec<Brought<DefinitionType>> broughtTypeDefs;

	Vec<Function*>         functions;
	Vec<Brought<Function>> broughtFunctions;

	Vec<PrerunFunction*>         prerunFunctions;
	Vec<Brought<PrerunFunction>> broughtPrerunFunctions;

	Vec<GenericFunction*>         genericFunctions;
	Vec<Brought<GenericFunction>> broughtGenericFunctions;

	Vec<GenericStructType*>         genericStructTypes;
	Vec<Brought<GenericStructType>> broughtGenericStructTypes;

	Vec<GenericToggleType*>         genericToggleTypes;
	Vec<Brought<GenericToggleType>> broughtGenericToggleTypes;

	Vec<GenericDefinitionType*>         genericTypeDefinitions;
	Vec<Brought<GenericDefinitionType>> broughtGenericTypeDefinitions;

	Vec<GlobalEntity*>         globalEntities;
	Vec<Brought<GlobalEntity>> broughtGlobalEntities;

	Vec<PrerunGlobal*>         prerunGlobals;
	Vec<Brought<PrerunGlobal>> broughtPrerunGlobals;

	Vec<Skill*>         skills;
	Vec<Brought<Skill>> broughtSkills;

	Vec<GenericSkill*>         genericSkills;
	Vec<Brought<GenericSkill>> broughtGenericSkills;

	Vec<DoneSkill*>         namedImplementations;
	Vec<Brought<DoneSkill>> broughtNamedImplementations;

	Vec<Region*>         regions;
	Vec<Brought<Region>> broughtRegions;

	Vec<EntityState*> entityEntries;

	Function* moduleInitialiser   = nullptr;
	Function* moduleDeinitialiser = nullptr;
	u64       nonConstantGlobals  = 0;

	HashSet<u64> integerBitwidths;
	HashSet<u64> unsignedBitwidths;

	HashSet<FloatTypeKind> floatKinds;

	bool isMatrixIntrinsicsUsed = false;

	Vec<Pair<Mod*, FileRangePtr>> fsBroughtMentions;

	Vec<ast::Node*> nodes;
	bool            hasMain = false;
	FilePath        llPath;
	Maybe<FilePath> objectFilePath;

	mutable llvm::Module*              llvmModule;
	mutable Vec<llvm::GlobalVariable*> otherGlobals;

	mutable Maybe<String> moduleForeignID;

	mutable bool linkPthread                  = false;
	mutable bool hasCreatedModules            = false;
	mutable bool hasHandledFilesystemBrings   = false;
	mutable bool hasCreatedEntities           = false;
	mutable bool hasUpdatedEntityDependencies = false;

	bool isCompiledToObject = false;
	bool isBundled          = false;

	void addMember(Mod* mod);

	void addNamedSubmodule(const Identifier& name, const String& _filename, ModuleType type,
	                       const VisibilityInfo& visib_info, ir::Ctx* irCtx);
	void closeSubmodule();

	bool should_be_named() const;

	static HashMap<InternalDependency, Function*> providedFunctions;

  public:
	~Mod();

	static Mod* create(const Identifier& name, const FilePath& filepath, const FilePath& basePath, ModuleType type,
	                   const VisibilityInfo& visib_info, ir::Ctx* irCtx);
	static Mod* create_submodule(Mod* parent, FilePath _filepath, FilePath basePath, Identifier name, ModuleType type,
	                             const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx);
	static Mod* create_file_mod(Mod* parent, FilePath _filepath, FilePath basePath, Identifier name, Vec<ast::Node*>,
	                            VisibilityInfo visibilityInfo, ir::Ctx* irCtx);
	static Mod* create_root_lib(Mod* parent, FilePath _filePath, FilePath basePath, Identifier name,
	                            Vec<ast::Node*> nodes, const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

	static bool has_provided_function(InternalDependency unit) { return providedFunctions.contains(unit); }

	static void add_provided_function(InternalDependency unit, Function* fnVal) { providedFunctions[unit] = fnVal; }

	static Function* get_provided_function(InternalDependency unit) { return providedFunctions[unit]; }

	static bool triple_is_equivalent(llvm::Triple const& first, llvm::Triple const& second);

	static Vec<Function*> collect_mod_initialisers();

	bool has_entity_with_name(String const& name) {
		for (auto ent : entityEntries) {
			if (ent->name.has_value() && ent->name->value == name) {
				return true;
			}
		}
		for (auto sub : submodules) {
			if (not sub->should_be_named()) {
				if (sub->has_entity_with_name(name)) {
					return true;
				}
			}
		}
		for (auto bMod : importedModules) {
			if (not bMod.is_named() && not bMod.get()->should_be_named()) {
				if (bMod.get()->has_entity_with_name(name)) {
					return true;
				}
			}
		}
		return false;
	}

	EntityState* add_entity(Maybe<Identifier> name, EntityType type, ast::IsEntity* node, EmitPhase maxPhase) {
		entityEntries.push_back(EntityState::create(name, type, EntityStatus::none, node, maxPhase));
		return entityEntries.back();
	}

	EntityState* get_entity(String const& name) {
		for (auto ent : entityEntries) {
			if (ent->name.has_value() && ent->name->value == name) {
				return ent;
			}
		}
		for (auto sub : submodules) {
			if (not sub->should_be_named()) {
				if (sub->has_entity_with_name(name)) {
					return sub->get_entity(name);
				}
			}
		}
		for (auto bMod : importedModules) {
			if (not bMod.is_named() && not bMod.get()->should_be_named()) {
				if (bMod.get()->has_entity_with_name(name)) {
					return bMod.get()->get_entity(name);
				}
			}
		}
		return nullptr;
	}

	void entity_name_check(ir::Ctx* irCtx, Identifier name, ir::EntityType entTy);

	ModuleType get_mod_type() const;
	String     get_full_name() const;
	String     get_referrable_name() const;
	String     get_writable_name() const;
	String     get_name() const;
	Identifier get_identifier() const;
	String     get_fullname_with_child(const String& name) const;
	Mod*       get_active();
	Mod*       get_parent_file();

	String get_file_path() const { return filePath.string(); }

	void         set_file_range(FileRangePtr fileRange);
	FileRangePtr get_file_range() const;

	Function* get_mod_initialiser(ir::Ctx* irCtx);
	bool      should_call_initialiser() const;
	void      add_non_const_global_counter();

	LinkNames get_link_names() const;

	bool is_parent_mod_of(Mod* other) const;
	bool has_parent_lib() const;
	Mod* get_closest_parent_lib();

	bool                    has_meta_info_key(String key) const;
	bool                    has_meta_info_key_in_parent(String key) const;
	bool                    is_in_foreign_mod_of_type(String id) const;
	Maybe<ir::PrerunValue*> get_meta_info_for_key(String key) const;
	Maybe<ir::PrerunValue*> get_meta_info_from_parent(String key) const;
	Maybe<String>           get_relevant_foreign_id() const;

	bool has_nth_parent(u32 n) const;
	Mod* get_nth_parent(u32 n);

	const VisibilityInfo& get_visibility() const;

	Function* create_function(Identifier const& name, bool isInline, Type* returnType, Vec<Argument> args,
	                          Maybe<Variadics> variadics, FileRangePtr fileRange, VisibilityInfo const& visibility,
	                          Maybe<llvm::GlobalValue::LinkageTypes> linkage, ir::Ctx* irCtx);

	bool is_submodule() const { return parent != nullptr; }

	bool has_submodules() const { return not submodules.empty(); }

	void add_dependency(ir::Mod* dep);

	bool has_integer_bitwidth(u64 bits) const {
		return (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) ||
		       integerBitwidths.contains(bits);
	}

	bool has_unsigned_bitwidth(u64 bits) const {
		return (bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64 || bits == 128) ||
		       unsignedBitwidths.contains(bits);
	}

	bool has_float_kind(FloatTypeKind kind) const {
		return (kind == FloatTypeKind::_32 || kind == FloatTypeKind::_64) || floatKinds.contains(kind);
	}

	void add_integer_bitwidth(u64 bits) { integerBitwidths.insert(bits); }

	void add_unsigned_bitwidth(u64 bits) { unsignedBitwidths.insert(bits); }

	void add_float_kind(FloatTypeKind kind) { floatKinds.insert(kind); }

	bool has_main_function() const { return hasMain; }

	void set_has_main_function() { hasMain = true; }

	std::set<String> get_all_object_files() const;

	std::set<String> get_all_linkable_libs() const;

	void add_filesystem_import_mention(ir::Mod* otherMod, FileRangePtr fileRange);

	Vec<Pair<Mod*, FileRangePtr>> const& get_fs_bring_mentions() const;

	// LIB

	bool               has_lib(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_lib(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_lib_in_imports(const String& name, const AccessInfo& reqInfo) const;
	Mod*               get_lib(const String& name, const AccessInfo& reqInfo);

	void open_lib_for_creation(const Identifier& name, const String& filename, const VisibilityInfo& visib_info,
	                           ir::Ctx* irCtx);
	void close_lib_after_creation();

	// FUNCTION

	bool               has_function(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_function(const String& name, Maybe<AccessInfo> reqInfo) const;
	Function*          get_function(const String& name, const AccessInfo& reqInfo);
	Pair<bool, String> has_function_in_imports(const String& name, const AccessInfo& reqInfo) const;

	// PRERUN FUNCTION

	bool               has_prerun_function(String const& name, AccessInfo reqInfo) const;
	bool               has_imported_prerun_function(String const& name, Maybe<AccessInfo> reqInfo) const;
	PrerunFunction*    get_prerun_function(String const& name, const AccessInfo& reqInfo);
	Pair<bool, String> has_prerun_function_in_imports(String const& name, AccessInfo const& reqInfo) const;

	// GENERIC FUNCTIONS

	bool               has_generic_function(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_generic_function(const String& name, Maybe<AccessInfo> reqInfo) const;
	GenericFunction*   get_generic_function(const String& name, const AccessInfo& reqInfo);
	Pair<bool, String> has_generic_function_in_imports(const String& name, const AccessInfo& reqInfo) const;

	// REGION

	bool               has_region(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_region(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_region_in_imports(const String& name, const AccessInfo& reqInfo) const;
	Region*            get_region(const String& name, const AccessInfo& reqInfo) const;

	// OPAQUE TYPES

	bool               has_opaque_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_opaque_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_opaque_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	OpaqueType*        get_opaque_type(const String& name, const AccessInfo& reqInfo) const;

	// STRUCT TYPE

	bool               has_struct_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	StructType*        get_struct_type(const String& name, const AccessInfo& reqInfo) const;

	// MIX TYPE

	bool               has_mix_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_mix_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_mix_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	MixType*           get_mix_type(const String& name, const AccessInfo& reqInfo) const;

	// TOGGLE TYPE

	bool               has_toggle_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_toggle_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_toggle_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	ToggleType*        get_toggle_type(const String& name, const AccessInfo& reqInfo) const;

	// CHOICE TYPE

	bool               has_choice_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_choice_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_choice_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	ChoiceType*        get_choice_type(const String& name, const AccessInfo& reqInfo) const;

	// FLAG TYPE

	bool               has_flag_type(const String& name, AccessInfo reqInfo) const;
	bool               has_brought_flag_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_flag_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	FlagType*          get_flag_type(const String& name, const AccessInfo& reqInfo) const;

	// GENERIC STRUCT TYPE

	bool               has_generic_struct_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_generic_struct_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_generic_struct_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	GenericStructType* get_generic_struct_type(const String& name, const AccessInfo& reqInfo);

	// GENERIC TOGGLE TYPE

	bool               has_generic_toggle_type(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_generic_toggle_type(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_generic_toggle_type_in_imports(const String& name, const AccessInfo& reqInfo) const;
	GenericToggleType* get_generic_toggle_type(const String& name, const AccessInfo& reqInfo);

	// GENERIC TYPEDEFS

	bool                   has_generic_type_def(const String& name, AccessInfo reqInfo) const;
	bool                   has_brought_generic_type_def(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String>     has_generic_type_def_in_imports(const String& name, const AccessInfo& reqInfo) const;
	GenericDefinitionType* get_generic_type_def(const String& name, const AccessInfo& reqInfo);

	// TYPEDEFS

	bool               has_type_definition(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_type_definition(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_type_definition_in_imports(const String& name, const AccessInfo& reqInfo) const;
	DefinitionType*    get_type_def(const String& name, const AccessInfo& reqInfo) const;

	// GLOBAL

	bool               has_global(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_global(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_global_in_imports(const String& name, const AccessInfo& reqInfo) const;
	GlobalEntity*      get_global(const String& name, const AccessInfo& reqInfo) const;

	// PRERUN GLOBAL

	bool               has_prerun_global(const String& name, AccessInfo reqInfo) const;
	bool               has_imported_prerun_global(const String& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_prerun_global_in_imports(const String& name, const AccessInfo& reqInfo) const;
	PrerunGlobal*      get_prerun_global(const String& name, const AccessInfo& reqInfo) const;

	// SKILLS

	bool               has_skill(String const& name, AccessInfo reqInfo) const;
	bool               has_brought_skill(String const& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_skill_in_imports(String const& name, AccessInfo const& reqInfo) const;
	Skill*             get_skill(String const& name, AccessInfo const& reqInfo) const;

	Vec<Skill*> const& get_skills_in_module() const { return skills; }

	Vec<Brought<Skill>> const& get_all_brought_skills() const { return broughtSkills; }

	// GENERIC SKILLS

	bool               has_generic_skill(String const& name, AccessInfo reqInfo) const;
	bool               has_brought_generic_skill(String const& name, Maybe<AccessInfo> reqInfo) const;
	Pair<bool, String> has_generic_skill_in_imports(String const& name, AccessInfo const& reqInfo) const;
	GenericSkill*      get_generic_skill(String const& name, AccessInfo const& reqInfo) const;

	// NAMED IMPLEMENTATIONS

	bool               has_named_implementation(String const& name, AccessInfo const& access) const;
	bool               has_brought_named_implementation(String const& name, Maybe<AccessInfo> access) const;
	Pair<bool, String> has_named_implementation_in_imports(String const& name, AccessInfo const& access) const;
	DoneSkill*         get_named_implementation(String const& name, AccessInfo const& access) const;

	// IMPORT

	bool               has_imported_mod(const String& name, Maybe<AccessInfo> reqInfo) const;
	Mod*               get_imported_mod(const String& name, const AccessInfo& reqInfo) const;
	Pair<bool, String> has_brought_mod_in_imports(const String& name, const AccessInfo& reqInfo) const;

	// BRING ENTITIES

	void import_module(Mod* other, const VisibilityInfo& _visib, Maybe<Identifier> bName = None);
	void import_struct_type(StructType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_opaque_type(OpaqueType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_mix_type(MixType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_toggle_type(ToggleType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_choice_type(ChoiceType* chTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_type_definition(DefinitionType* dTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_function(Function* fn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_prerun_function(PrerunFunction* preFn, VisibilityInfo const& visib, Maybe<Identifier> bName = None);
	void import_region(Region* reg, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_global(GlobalEntity* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_prerun_global(PrerunGlobal* preGlobal, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void import_generic_struct_type(GenericStructType* gCTy, const VisibilityInfo& visib,
	                                Maybe<Identifier> bName = None);
	void import_generic_toggle_type(GenericToggleType* gCTy, const VisibilityInfo& visib,
	                                Maybe<Identifier> bName = None);
	void import_generic_function(GenericFunction* gFn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
	void bring_generic_type_definition(GenericDefinitionType* gTDef, VisibilityInfo const& visib,
	                                   Maybe<Identifier> bName = None);
	void bring_skill(Skill* skill, VisibilityInfo const& visib, Maybe<Identifier> bName = None);
	void bring_generic_skill(GenericSkill* skill, VisibilityInfo const& visib, Maybe<Identifier> bName = None);

	void set_matrix_intrinsic_used() { isMatrixIntrinsicsUsed = true; }

	FilePath        get_resolved_output_path(const String& extension, Ctx* irCtx);
	llvm::Module*   get_llvm_module() const;
	Maybe<FilePath> find_static_library_path(String libName) const;

	bool find_clang_path(Ctx* irCtx);
	bool find_windows_sdk_paths(Ctx* irCtx);
	bool find_windows_toolchain_libs(Ctx* irCtx, bool findMSVCLibPath, bool findATLMFCLibPath, bool findUCRTLibPath,
	                                 bool findUMLibPath);

	static void find_native_library_paths();

	void node_create_modules(Ctx* irCtx);
	void node_handle_fs_brings(Ctx* irCtx);
	void node_create_entities(Ctx* irCtx);
	void node_update_dependencies(Ctx* irCtx);

	void setup_llvm_file(Ctx* irCtx);
	void compile_to_object(Ctx* irCtx, llvm::PassBuilder passBuilder);
	void handle_native_libs(Ctx* irCtx);
	void bundle_modules(Ctx* irCtx);

	/// This returns the name of the linked function or symbol
	/// Even known units like printf can have a different name for the underlying function, especially in freehosting
	/// environments
	String link_internal_dependency(InternalDependency nval, Ctx* irCtx, FileRangePtr fileRange);

	llvm::Function* link_intrinsic(IntrinsicID intr);
};

} // namespace qat::ir

#endif
