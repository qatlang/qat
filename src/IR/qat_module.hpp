#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../utils/file_range.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./static_member.hpp"
#include "./types/array.hpp"
#include "./types/core_type.hpp"
#include "./types/float.hpp"
#include "./types/mix.hpp"
#include "./types/void.hpp"
#include "types/definition.hpp"
#include "llvm/IR/LLVMContext.h"
#include <vector>

namespace qat::ast {
class Node;
class Lib;
class Box;
} // namespace qat::ast

namespace qat::IR {

class Context;

enum class ModuleType { lib, box, file, folder };
enum class NativeUnit {
  printf,
  malloc,
  free,
  realloc,
  pthreadCreate,
  pthreadJoin
};

class QatModule : public Uniq {
  friend class CoreType;
  friend class MixType;
  friend class DefinitionType;
  friend class GlobalEntity;
  friend class ast::Lib;
  friend class ast::Box;
  friend class TemplateFunction;
  friend class TemplateCoreType;

public:
  QatModule(String _name, fs::path _filePath, fs::path _basePath,
            ModuleType _type, const utils::VisibilityInfo &_visibility,
            llvm::LLVMContext &ctx);

private:
  String                         name;
  ModuleType                     moduleType;
  fs::path                       filePath;
  fs::path                       basePath;
  utils::VisibilityInfo          visibility;
  QatModule                     *parent;
  QatModule                     *active;
  Vec<QatModule *>               submodules;
  Vec<Brought<QatModule>>        broughtModules;
  Vec<CoreType *>                coreTypes;
  Vec<Brought<CoreType>>         broughtCoreTypes;
  Vec<MixType *>                 mixTypes;
  Vec<Brought<MixType>>          broughtMixTypes;
  Vec<DefinitionType *>          typeDefs;
  Vec<Brought<DefinitionType>>   broughtTypeDefs;
  Vec<Function *>                functions;
  Vec<Brought<Function>>         broughtFunctions;
  Vec<TemplateFunction *>        templateFunctions;
  Vec<Brought<TemplateFunction>> broughtTemplateFunctions;
  Vec<TemplateCoreType *>        templateCoreTypes;
  Vec<Brought<TemplateCoreType>> broughtTemplateCoreTypes;
  Vec<GlobalEntity *>            globalEntities;
  Vec<Brought<GlobalEntity>>     broughtGlobalEntities;
  Function                      *globalInitialiser;

  Vec<u64>           integerBitwidths;
  Vec<u64>           unsignedBitwidths;
  Vec<FloatTypeKind> floatKinds;

  Vec<String>           content;
  Vec<ast::Node *>      nodes;
  mutable bool          isEmitted = false;
  mutable llvm::Module *llvmModule;

  void       addSubmodule(const String &name, const String &_filename,
                          ModuleType type, const utils::VisibilityInfo &visib_info,
                          llvm::LLVMContext &ctx);
  void       closeSubmodule();
  useit bool shouldPrefixName() const;

public:
  ~QatModule();

  useit static QatModule *Create(const String &name, const fs::path &filepath,
                                 const fs::path &basePath, ModuleType type,
                                 const utils::VisibilityInfo &visib_info,
                                 llvm::LLVMContext           &ctx);
  useit static QatModule *
  CreateSubmodule(QatModule *parent, fs::path _filepath, fs::path basePath,
                  String name, ModuleType type,
                  const utils::VisibilityInfo &visibilityInfo,
                  llvm::LLVMContext           &ctx);
  useit static QatModule *CreateFile(QatModule *parent, fs::path _filepath,
                                     fs::path basePath, String name,
                                     Vec<String> content, Vec<ast::Node *>,
                                     utils::VisibilityInfo visibilityInfo,
                                     llvm::LLVMContext    &ctx);

  useit ModuleType getModuleType() const;
  useit String     getFullName() const;
  useit String     getWritableName() const;
  useit String     getName() const;
  useit String     getFullNameWithChild(const String &name) const;
  useit QatModule *getActive();
  useit QatModule *getParentFile();
  useit String     getFilePath() const;
  useit Function  *getGlobalInitialiser(IR::Context *ctx);

  useit bool       hasClosestParentLib() const;
  useit QatModule *getClosestParentLib();
  useit bool       hasClosestParentBox() const;
  useit QatModule *getClosestParentBox();
  useit Pair<unsigned, String> resolveNthParent(const String &name) const;
  useit bool                   hasNthParent(u32 n) const;
  useit QatModule             *getNthParent(u32 n);
  useit const utils::VisibilityInfo &getVisibility() const;
  useit Function *createFunction(const String &name, QatType *returnType,
                                 bool isReturnTypeVariable, bool isAsync,
                                 Vec<Argument> args, bool isVariadic,
                                 const utils::FileRange         &fileRange,
                                 const utils::VisibilityInfo    &visibility,
                                 llvm::GlobalValue::LinkageTypes linkage,
                                 llvm::LLVMContext              &ctx);
  useit bool      isSubmodule() const;

  useit bool hasIntegerBitwidth(u64 bits) const;
  void       addIntegerBitwidth(u64 bits);
  useit bool hasUnsignedBitwidth(u64 bits) const;
  void       addUnsignedBitwidth(u64 bits);
  useit bool hasFloatKind(FloatTypeKind kind) const;
  void       addFloatKind(FloatTypeKind kind);

  // LIB

  useit bool       hasLib(const String &name) const;
  useit bool       hasBroughtLib(const String &name) const;
  useit            Pair<bool, String>
                   hasAccessibleLibInImports(const String               &name,
                                             const utils::RequesterInfo &reqInfo) const;
  useit QatModule *getLib(const String               &name,
                          const utils::RequesterInfo &reqInfo);
  void             openLib(const String &name, const String &filename,
                           const utils::VisibilityInfo &visib_info, llvm::LLVMContext &ctx);
  void             closeLib();

  // BOX

  useit bool       hasBox(const String &name) const;
  useit bool       hasBroughtBox(const String &name) const;
  useit QatModule *getBox(const String               &name,
                          const utils::RequesterInfo &reqInfo);
  useit            Pair<bool, String>
                   hasAccessibleBoxInImports(const String               &name,
                                             const utils::RequesterInfo &reqInfo) const;
  void openBox(const String &name, Maybe<utils::VisibilityInfo> visib_info);
  void closeBox();

  // FUNCTION

  useit bool      hasFunction(const String &name) const;
  useit bool      hasBroughtFunction(const String &name) const;
  useit Function *getFunction(const String               &name,
                              const utils::RequesterInfo &reqInfo);
  useit           Pair<bool, String>
                  hasAccessibleFunctionInImports(const String               &name,
                                                 const utils::RequesterInfo &reqInfo) const;

  // TEMPLATE FUNCTIONS

  useit bool hasTemplateFunction(const String &name) const;
  useit bool hasBroughtTemplateFunction(const String &name) const;
  useit TemplateFunction *
  getTemplateFunction(const String &name, const utils::RequesterInfo &reqInfo);
  useit Pair<bool, String> hasAccessibleTemplateFunctionInImports(
      const String &name, const utils::RequesterInfo &reqInfo) const;

  // CORE TYPE

  useit bool      hasCoreType(const String &name) const;
  useit bool      hasBroughtCoreType(const String &name) const;
  useit           Pair<bool, String>
                  hasAccessibleCoreTypeInImports(const String               &name,
                                                 const utils::RequesterInfo &reqInfo) const;
  useit CoreType *getCoreType(const String               &name,
                              const utils::RequesterInfo &reqInfo) const;

  // MIX TYPE

  useit bool     hasMixType(const String &name) const;
  useit bool     hasBroughtMixType(const String &name) const;
  useit          Pair<bool, String>
                 hasAccessibleMixTypeInImports(const String               &name,
                                               const utils::RequesterInfo &reqInfo) const;
  useit MixType *getMixType(const String               &name,
                            const utils::RequesterInfo &reqInfo) const;

  // TEMPLATE FUNCTIONS

  useit bool hasTemplateCoreType(const String &name) const;
  useit bool hasBroughtTemplateCoreType(const String &name) const;
  useit Pair<bool, String> hasAccessibleTemplateCoreTypeInImports(
      const String &name, const utils::RequesterInfo &reqInfo) const;
  useit TemplateCoreType *
  getTemplateCoreType(const String &name, const utils::RequesterInfo &reqInfo);

  // TYPEDEF
  useit bool            hasTypeDef(const String &name) const;
  useit bool            hasBroughtTypeDef(const String &name) const;
  useit                 Pair<bool, String>
                        hasAccessibleTypeDefInImports(const String               &name,
                                                      const utils::RequesterInfo &reqInfo) const;
  useit DefinitionType *getTypeDef(const String               &name,
                                   const utils::RequesterInfo &reqInfo) const;

  // GLOBAL ENTITY

  useit bool hasGlobalEntity(const String &name) const;
  useit bool hasBroughtGlobalEntity(const String &name) const;
  useit      Pair<bool, String>
             hasAccessibleGlobalEntityInImports(const String               &name,
                                                const utils::RequesterInfo &reqInfo) const;
  useit GlobalEntity *
  getGlobalEntity(const String               &name,
                  const utils::RequesterInfo &reqInfo) const;

  // IMPORT

  useit bool       hasImport(const String &name) const;
  useit QatModule *getImport(const String &name) const;
  useit            Pair<bool, String>
                   hasAccessibleNamedImportInImports(const String               &name,
                                                     const utils::RequesterInfo &reqInfo) const;

  // BRING ENTITIES

  void bring_module(QatModule *other, const utils::VisibilityInfo &_visibility);
  void bring_named_module(const String &_name, QatModule *other,
                          const utils::VisibilityInfo &_visibility);
  void bring_entity(const String                &name,
                    const utils::VisibilityInfo &_visibility);
  void bring_named_entity(const String &name, const String &entity,
                          const utils::VisibilityInfo &_visibility);
  useit llvm::GlobalVariable       *
  get_global_variable(String name, utils::RequesterInfo &req_info);
  useit fs::path getResolvedOutputPath(const String &extension) const;
  useit llvm::Module *getLLVMModule() const;

  bool      areNodesEmitted() const;
  void      createModules(IR::Context *ctx);
  void      defineTypes(IR::Context *ctx);
  void      defineNodes(IR::Context *ctx);
  void      emitNodes(IR::Context *ctx);
  void      exportJsonFromAST() const;
  void      linkNative(NativeUnit nval);
  void      finaliseModule();
  nuo::Json toJson() const;
};

} // namespace qat::IR

#endif