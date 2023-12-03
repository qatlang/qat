#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../memory_tracker.hpp"
#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./types/core_type.hpp"
#include "./types/definition.hpp"
#include "./types/float.hpp"
#include "./types/mix.hpp"
#include "entity_overview.hpp"
#include "link_names.hpp"
#include "lld/Common/Driver.h"
#include "types/definition.hpp"
#include "llvm/IR/LLVMContext.h"
#include <vector>

LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(wasm)

namespace qat::ast {
class Node;
class Lib;
class Box;
class ModInfo;
class BringPaths;
} // namespace qat::ast

namespace qat::IR {

class Context;

enum class ModuleType { lib, box, file, folder };
enum class NativeUnit { printf, malloc, free, realloc, pthreadCreate, pthreadJoin, pthreadExit, pthreadAttrInit };
enum class IntrinsicID { vaStart, vaEnd, vaCopy };

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

class ModuleInfo {
  friend class ast::ModInfo;
  friend class QatModule;

private:
  Maybe<String>    outputName;
  Deque<LibToLink> nativeLibsToLink;
  bool             linkPthread = false;
  Maybe<String>    foreignID;
  Maybe<String>    alternativeName;

  void addLibToLink(LibToLink libVal) {
    for (const auto& lib : nativeLibsToLink) {
      if (lib == libVal) {
        return;
      }
    }
    nativeLibsToLink.push_back(libVal);
  }
  useit bool   isForeign() const { return foreignID.has_value(); }
  useit bool   isForeignC() const { return foreignID.has_value() && (foreignID.value() == "C"); }
  useit bool   isForeignCPP() const { return foreignID.has_value() && (foreignID.value() == "C++"); }
  useit String foreignIdentity() const { return foreignID.value_or(""); }
  useit bool   hasAlternativeName() const { return alternativeName.has_value(); }
  useit String getAlternativeName() const { return alternativeName.value_or(""); }

public:
  ModuleInfo() = default;

  operator JsonValue() {
    return Json()
        ._("linksPThread", linkPthread)
        ._("hasForeignID", foreignID.has_value())
        ._("hasOutputName", outputName.has_value())
        ._("outputName", outputName.has_value() ? outputName.value() : JsonValue())
        ._("foreignID", foreignID.has_value() ? foreignID.value() : JsonValue());
  }
};

class QatModule final : public Uniq, public EntityOverview {
  friend class Region;
  friend class OpaqueType;
  friend class CoreType;
  friend class MixType;
  friend class ChoiceType;
  friend class DefinitionType;
  friend class GlobalEntity;
  friend class ast::Lib;
  friend class ast::Box;
  friend class ast::ModInfo;
  friend class ast::BringPaths;
  friend class GenericFunction;
  friend class GenericCoreType;
  friend class GenericDefinitionType;

public:
  QatModule(Identifier _name, fs::path _filePath, fs::path _basePath, ModuleType _type,
            const VisibilityInfo& _visibility, IR::Context* ctx);

  static Vec<QatModule*> allModules;

  useit static bool       hasFileModule(const fs::path& fPath);
  useit static bool       hasFolderModule(const fs::path& fPath);
  useit static QatModule* getFileModule(const fs::path& fPath);
  useit static QatModule* getFolderModule(const fs::path& fPath);

private:
  Identifier                          name;
  ModuleType                          moduleType;
  bool                                rootLib = false;
  ModuleInfo                          moduleInfo;
  bool                                isModuleInfoProvided = false;
  fs::path                            filePath;
  fs::path                            basePath;
  VisibilityInfo                      visibility;
  QatModule*                          parent = nullptr;
  QatModule*                          active = nullptr;
  Vec<QatModule*>                     submodules;
  Vec<Brought<QatModule>>             broughtModules;
  Deque<OpaqueType*>                  opaqueTypes;
  Vec<Brought<OpaqueType>>            broughtOpaqueTypes;
  Vec<Brought<OpaqueType>>            broughtGenericOpaqueTypes;
  Vec<CoreType*>                      coreTypes;
  Vec<Brought<CoreType>>              broughtCoreTypes;
  Vec<ChoiceType*>                    choiceTypes;
  Vec<Brought<ChoiceType>>            broughtChoiceTypes;
  Vec<MixType*>                       mixTypes;
  Vec<Brought<MixType>>               broughtMixTypes;
  Vec<DefinitionType*>                typeDefs;
  Vec<Brought<DefinitionType>>        broughtTypeDefs;
  Vec<Function*>                      functions;
  Vec<Brought<Function>>              broughtFunctions;
  Vec<GenericFunction*>               genericFunctions;
  Vec<Brought<GenericFunction>>       broughtGenericFunctions;
  Vec<GenericCoreType*>               genericCoreTypes;
  Vec<Brought<GenericCoreType>>       broughtGenericCoreTypes;
  Vec<GenericDefinitionType*>         genericTypeDefinitions;
  Vec<Brought<GenericDefinitionType>> broughtGenericTypeDefinitions;
  Vec<GlobalEntity*>                  globalEntities;
  Vec<Brought<GlobalEntity>>          broughtGlobalEntities;
  Vec<Region*>                        regions;
  Vec<Brought<Region>>                broughtRegions;
  Function*                           moduleInitialiser   = nullptr;
  Function*                           moduleDeinitialiser = nullptr;
  u64                                 nonConstantGlobals  = 0;

  Vec<u64>           integerBitwidths;
  Vec<u64>           unsignedBitwidths;
  Vec<FloatTypeKind> floatKinds;

  Vec<Pair<QatModule*, FileRange>> fsBroughtMentions;

  Vec<String>           content;
  Vec<ast::Node*>       nodes;
  mutable llvm::Module* llvmModule;
  bool                  hasMain = false;
  fs::path              llPath;
  Maybe<fs::path>       objectFilePath;
  mutable bool          hasCreatedModules          = false;
  mutable bool          hasHandledFilesystemBrings = false;
  mutable bool          hasHandledBrings           = false;
  mutable bool          hasDefinedTypes            = false;
  mutable bool          hasDefinedNodes            = false;
  mutable bool          isEmitted                  = false;
  mutable bool          isOverviewOutputted        = false;
  bool                  isCompiledToObject         = false;
  bool                  isBundled                  = false;

  void addMember(QatModule* mod);

  void       addNamedSubmodule(const Identifier& name, const String& _filename, ModuleType type,
                               const VisibilityInfo& visib_info, IR::Context* ctx);
  void       closeSubmodule();
  useit bool shouldPrefixName() const;

public:
  ~QatModule();

  useit static QatModule* Create(const Identifier& name, const fs::path& filepath, const fs::path& basePath,
                                 ModuleType type, const VisibilityInfo& visib_info, IR::Context* ctx);
  useit static QatModule* CreateSubmodule(QatModule* parent, fs::path _filepath, fs::path basePath, Identifier name,
                                          ModuleType type, const VisibilityInfo& visibilityInfo, IR::Context* ctx);
  useit static QatModule* CreateFileMod(QatModule* parent, fs::path _filepath, fs::path basePath, Identifier name,
                                        Vec<String> content, Vec<ast::Node*>, VisibilityInfo visibilityInfo,
                                        IR::Context* ctx);
  useit static QatModule* CreateRootLib(QatModule* parent, fs::path _filePath, fs::path basePath, Identifier name,
                                        Vec<String> content, Vec<ast::Node*> nodes, const VisibilityInfo& visibInfo,
                                        IR::Context* ctx);

  static bool           tripleIsEquivalent(llvm::Triple const& first, llvm::Triple const& second);
  static Vec<Function*> collectModuleInitialisers();

  useit ModuleType getModuleType() const;
  useit String     getFullName() const;
  useit String     getWritableName() const;
  useit String     getName() const;
  useit Identifier getNameIdentifier() const;
  useit String     getFullNameWithChild(const String& name) const;
  useit QatModule* getActive();
  useit QatModule* getParentFile();
  useit String     getFilePath() const;
  useit Function*  getGlobalInitialiser(IR::Context* ctx);
  void             incrementNonConstGlobalCounter();
  useit bool       shouldCallInitialiser() const;
  void             setFileRange(FileRange fileRange);
  FileRange        getFileRange() const;

  useit LinkNames getLinkNames() const;

  useit bool       isParentModuleOf(QatModule* other) const;
  useit bool       hasClosestParentLib() const;
  useit QatModule* getClosestParentLib();
  useit bool       hasClosestParentBox() const;
  useit QatModule* getClosestParentBox();
  useit bool       isInForeignModuleOfType(String id) const;
  useit bool       hasNthParent(u32 n) const;
  useit QatModule* getNthParent(u32 n);

  useit const VisibilityInfo& getVisibility() const;
  useit Function* createFunction(const Identifier& name, QatType* returnType, bool isAsync, Vec<Argument> args,
                                 bool isVariadic, const FileRange& fileRange, const VisibilityInfo& visibility,
                                 Maybe<llvm::GlobalValue::LinkageTypes> linkage, IR::Context* ctx);
  useit bool      isSubmodule() const;
  useit bool      hasSubmodules() const;

  useit bool hasIntegerBitwidth(u64 bits) const;
  void       addIntegerBitwidth(u64 bits);
  useit bool hasUnsignedBitwidth(u64 bits) const;
  void       addUnsignedBitwidth(u64 bits);
  useit bool hasFloatKind(FloatTypeKind kind) const;
  void       addFloatKind(FloatTypeKind kind);
  useit bool hasMainFn() const;
  void       setHasMainFn();

  void addFilesystemBroughtMention(IR::QatModule* otherMod, const FileRange& fileRange);
  Vec<Pair<QatModule*, FileRange>> const& getFilesystemBroughtMentions() const;

  void updateOverview() final;
  void outputAllOverview(Vec<JsonValue>& modulesJson, Vec<JsonValue>& functionsJson,
                         Vec<JsonValue>& genericFunctionsJson, Vec<JsonValue>& genericCoreTypesJson,
                         Vec<JsonValue>& coreTypesJson, Vec<JsonValue>& mixTypesJson, Vec<JsonValue>& regionJson,
                         Vec<JsonValue>& choiceJson, Vec<JsonValue>& defsJson);

  // LIB

  useit bool hasLib(const String& name) const;
  useit bool hasBroughtLib(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleLibInImports(const String& name, const AccessInfo& reqInfo) const;
  useit QatModule*         getLib(const String& name, const AccessInfo& reqInfo);
  void openLib(const Identifier& name, const String& filename, const VisibilityInfo& visib_info, IR::Context* ctx);
  void closeLib();

  // BOX

  useit bool       hasBox(const String& name) const;
  useit bool       hasBroughtBox(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit QatModule* getBox(const String& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> hasAccessibleBoxInImports(const String& name, const AccessInfo& reqInfo) const;
  void                     openBox(const Identifier& name, Maybe<VisibilityInfo> visib_info, IR::Context* ctx);
  void                     closeBox();

  // FUNCTION

  useit bool      hasFunction(const String& name) const;
  useit bool      hasBroughtFunction(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Function* getFunction(const String& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> hasAccessibleFunctionInImports(const String& name, const AccessInfo& reqInfo) const;

  // GENERIC FUNCTIONS

  useit bool             hasGenericFunction(const String& name) const;
  useit bool             hasBroughtGenericFunction(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit GenericFunction* getGenericFunction(const String& name, const AccessInfo& reqInfo);
  useit Pair<bool, String> hasAccessibleGenericFunctionInImports(const String& name, const AccessInfo& reqInfo) const;

  // REGION

  useit bool hasRegion(const String& name) const;
  useit bool hasBroughtRegion(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleRegionInImports(const String& name, const AccessInfo& reqInfo) const;
  useit Region*            getRegion(const String& name, const AccessInfo& reqInfo) const;

  // OPAQUE TYPES

  useit bool hasOpaqueType(const String& name) const;
  useit bool hasBroughtOpaqueType(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleOpaqueTypeInImports(const String& name, const AccessInfo& reqInfo) const;
  useit OpaqueType*        getOpaqueType(const String& name, const AccessInfo& reqInfo) const;

  // CORE TYPE

  useit bool hasCoreType(const String& name) const;
  useit bool hasBroughtCoreType(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleCoreTypeInImports(const String& name, const AccessInfo& reqInfo) const;
  useit CoreType*          getCoreType(const String& name, const AccessInfo& reqInfo) const;

  // MIX TYPE

  useit bool hasMixType(const String& name) const;
  useit bool hasBroughtMixType(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleMixTypeInImports(const String& name, const AccessInfo& reqInfo) const;
  useit MixType*           getMixType(const String& name, const AccessInfo& reqInfo) const;

  // CHOICE TYPE

  useit bool hasChoiceType(const String& name) const;
  useit bool hasBroughtChoiceType(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleChoiceTypeInImports(const String& name, const AccessInfo& reqInfo) const;
  useit ChoiceType*        getChoiceType(const String& name, const AccessInfo& reqInfo) const;

  // GENERIC CORE TYPES

  useit bool hasGenericCoreType(const String& name) const;
  useit bool hasBroughtGenericCoreType(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleGenericCoreTypeInImports(const String& name, const AccessInfo& reqInfo) const;
  useit GenericCoreType*   getGenericCoreType(const String& name, const AccessInfo& reqInfo);

  // GENERIC CORE TYPES

  useit bool hasGenericTypeDef(const String& name) const;
  useit bool hasBroughtGenericTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleGenericTypeDefInImports(const String& name, const AccessInfo& reqInfo) const;
  useit GenericDefinitionType* getGenericTypeDef(const String& name, const AccessInfo& reqInfo);

  // TYPEDEF
  useit bool hasTypeDef(const String& name) const;
  useit bool hasBroughtTypeDef(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleTypeDefInImports(const String& name, const AccessInfo& reqInfo) const;
  useit DefinitionType*    getTypeDef(const String& name, const AccessInfo& reqInfo) const;

  // GLOBAL ENTITY

  useit bool hasGlobalEntity(const String& name) const;
  useit bool hasBroughtGlobalEntity(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit Pair<bool, String> hasAccessibleGlobalEntityInImports(const String& name, const AccessInfo& reqInfo) const;
  useit GlobalEntity*      getGlobalEntity(const String& name, const AccessInfo& reqInfo) const;

  // IMPORT

  useit bool       hasBroughtModule(const String& name, Maybe<AccessInfo> reqInfo) const;
  useit QatModule* getBroughtModule(const String& name, const AccessInfo& reqInfo) const;
  useit Pair<bool, String> hasAccessibleBroughtModuleInImports(const String& name, const AccessInfo& reqInfo) const;

  // BRING ENTITIES

  void bringModule(QatModule* other, const VisibilityInfo& _visib, Maybe<Identifier> bName = None);
  void bringCoreType(CoreType* cTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringGenericCoreType(GenericCoreType* gCTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringMixType(MixType* mTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringChoiceType(ChoiceType* chTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringTypeDefinition(DefinitionType* dTy, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringFunction(Function* fn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringGenericFunction(GenericFunction* gFn, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringRegion(Region* reg, const VisibilityInfo& visib, Maybe<Identifier> bName = None);
  void bringGlobalEntity(GlobalEntity* gEnt, const VisibilityInfo& visib, Maybe<Identifier> bName = None);

  useit fs::path getResolvedOutputPath(const String& extension, IR::Context* ctx);
  useit llvm::Module* getLLVMModule() const;

  bool areNodesEmitted() const;
  void createModules(IR::Context* ctx);
  void handleFilesystemBrings(IR::Context* ctx);
  void handleBrings(IR::Context* ctx);
  void defineTypes(IR::Context* ctx);
  void defineNodes(IR::Context* ctx);
  void emitNodes(IR::Context* ctx);
  void compileToObject(IR::Context* ctx);
  void bundleLibs(IR::Context* ctx);
  void exportJsonFromAST(IR::Context* ctx);
  void linkNative(NativeUnit nval);
  void linkIntrinsic(IntrinsicID intr);
  void finaliseModule();
  Json toJson() const;
};

} // namespace qat::IR

#endif
