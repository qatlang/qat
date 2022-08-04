#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../utils/file_range.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./definable.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./static_member.hpp"
#include "./types/array.hpp"
#include "./types/core_type.hpp"
#include "./types/float.hpp"
#include "./types/void.hpp"
#include "emittable.hpp"
#include <vector>

namespace qat::ast {
class Node;
}

namespace qat::IR {

enum class ModuleType { lib, box, file, folder };

class QatModule : public Definable {
public:
  QatModule(const String &_name, const fs::path &_filePath,
            const fs::path &_basePath, ModuleType _type,
            const utils::VisibilityInfo &_visibility);

private:
  String                     name;
  ModuleType                 moduleType;
  fs::path                   filePath;
  fs::path                   basePath;
  utils::VisibilityInfo      visibility;
  QatModule                 *parent;
  QatModule                 *active;
  Vec<QatModule *>           submodules;
  Vec<Brought<QatModule>>    broughtModules;
  Vec<CoreType *>            coreTypes;
  Vec<Brought<CoreType>>     broughtCoreTypes;
  Vec<Function *>            functions;
  Vec<Brought<Function>>     broughtFunctions;
  Vec<GlobalEntity *>        globalEntities;
  Vec<Brought<GlobalEntity>> broughtGlobalEntities;
  Vec<Function *>            functionsToLink;

  bool shouldPrefixName() const;

  void addSubmodule(const String &name, const String &_filename,
                    ModuleType type, const utils::VisibilityInfo &visib_info);

  void closeSubmodule();

  Vec<ast::Node *> nodes;

  mutable bool isEmitted = false;

  mutable llvm::Module *llvmModule;

public:
  ~QatModule();

  static QatModule *Create(const String &name, const fs::path &filepath,
                           const fs::path &basePath, ModuleType type,
                           const utils::VisibilityInfo &visib_info);

  static QatModule *
  CreateSubmodule(QatModule *parent, fs::path _filepath, fs::path basePath,
                  String name, ModuleType type,
                  const utils::VisibilityInfo &visibilityInfo);

  static QatModule *CreateFile(QatModule *parent, fs::path _filepath,
                               fs::path basePath, String name, Vec<ast::Node *>,
                               utils::VisibilityInfo visibilityInfo);

  String getFullName() const;

  String getName() const;

  String getFullNameWithChild(const String &name) const;

  QatModule *getActive();

  QatModule *getParentFile();

  Pair<unsigned, String> resolveNthParent(const String &name) const;

  bool hasNthParent(u32 n) const;

  QatModule *getNthParent(u32 n);

  const utils::VisibilityInfo &getVisibility() const;

  Function *createFunction(const String &name, QatType *returnType,
                           bool isReturnTypeVariable, bool isAsync,
                           Vec<Argument> args, bool isVariadic,
                           const utils::FileRange      &fileRange,
                           const utils::VisibilityInfo &visibility);

  bool isSubmodule() const;

  // LIB

  bool hasLib(const String &name) const;

  bool hasBroughtLib(const String &name) const;

  Pair<bool, String>
  hasAccessibleLibInImports(const String               &name,
                            const utils::RequesterInfo &reqInfo) const;

  QatModule *getLib(const String &name, const utils::RequesterInfo &reqInfo);

  void openLib(const String &name, const String &filename,
               const utils::VisibilityInfo &visib_info);

  void closeLib();

  // BOX

  bool hasBox(const String &name) const;

  bool hasBroughtBox(const String &name) const;

  QatModule *getBox(const String &name, const utils::RequesterInfo &reqInfo);

  Pair<bool, String>
  hasAccessibleBoxInImports(const String               &name,
                            const utils::RequesterInfo &reqInfo) const;

  void openBox(const String &name, Maybe<utils::VisibilityInfo> visib_info);

  void closeBox();

  // FUNCTION

  bool hasFunction(const String &name) const;

  bool hasBroughtFunction(const String &name) const;

  Function *getFunction(const String               &name,
                        const utils::RequesterInfo &reqInfo);

  Pair<bool, String>
  hasAccessibleFunctionInImports(const String               &name,
                                 const utils::RequesterInfo &reqInfo) const;

  // CORE TYPE

  bool hasCoreType(const String &name) const;

  bool hasBroughtCoreType(const String &name) const;

  Pair<bool, String>
  hasAccessibleCoreTypeInImports(const String               &name,
                                 const utils::RequesterInfo &reqInfo) const;

  CoreType *getCoreType(const String               &name,
                        const utils::RequesterInfo &reqInfo) const;

  // GLOBAL ENTITY

  bool hasGlobalEntity(const String &name) const;

  bool hasBroughtGlobalEntity(const String &name) const;

  Pair<bool, String>
  hasAccessibleGlobalEntityInImports(const String               &name,
                                     const utils::RequesterInfo &reqInfo) const;

  GlobalEntity *getGlobalEntity(const String               &name,
                                const utils::RequesterInfo &reqInfo) const;

  GlobalEntity *createGlobalEntity();

  // IMPORT

  bool hasImport(const String &name) const;

  QatModule *getImport(const String &name) const;

  Pair<bool, String>
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

  llvm::GlobalVariable *get_global_variable(String                name,
                                            utils::RequesterInfo &req_info);

  bool areNodesEmitted() const;

  fs::path getResolvedOutputPath(const String &extension) const;

  void addFunctionToLink(Function *fun);

  void emitNodes(IR::Context *ctx) const;

  void exportJsonFromAST() const;

  void throw_error(String message, utils::FileRange fileRange);

  // Emitters

  // LLVM Definition function
  void defineLLVM(llvmHelper &help) const override;

  // CPP Definition function
  void defineCPP(cpp::File &file) const override;

  // LLVM Emitter function
  llvm::Module &getLLVMModule(llvmHelper &help) const;

  // CPP Emitter function
  void emitCPP(cpp::File &file) const;

  // JSON emitter function
  nuo::Json toJson() const;
};

} // namespace qat::IR

#endif