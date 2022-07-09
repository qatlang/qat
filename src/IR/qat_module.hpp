#ifndef QAT_IR_QAT_MODULE_HPP
#define QAT_IR_QAT_MODULE_HPP

#include "../utils/file_placement.hpp"
#include "../utils/visibility.hpp"
#include "./brought.hpp"
#include "./function.hpp"
#include "./global_entity.hpp"
#include "./static_member.hpp"
#include "./types/array.hpp"
#include "./types/core_type.hpp"
#include "./types/float.hpp"
#include "./types/void.hpp"
#include <vector>

namespace qat {
namespace IR {

enum class ModuleType { lib, box, file, folder };

class QatModule {
public:
  QatModule(std::string _name, fs::path _filepath, ModuleType _type,
            utils::VisibilityInfo _visibility);

private:
  // Name of the module. This will contribute to the names of its
  // submodules, if this module is not a file or folder
  std::string name;

  // Type of the module
  ModuleType moduleType;

  // Path of the file of the module
  std::string filePath;

  // Visibility of the module
  utils::VisibilityInfo visibility;

  // Parent module of this module
  QatModule *parent;

  // Active module
  QatModule *active;

  std::vector<QatModule *> submodules;

  std::vector<Brought<QatModule>> broughtModules;

  std::vector<CoreType *> coreTypes;

  std::vector<Brought<CoreType>> broughtCoreTypes;

  std::vector<Function *> functions;

  std::vector<Brought<Function>> broughtFunctions;

  std::vector<GlobalEntity *> globalEntities;

  std::vector<Brought<GlobalEntity>> broughtGlobalEntities;

  static QatModule *CreateSubmodule(QatModule *parent, std::string _filepath,
                                    std::string name, ModuleType type,
                                    utils::VisibilityInfo visib_info);

  bool shouldPrefixName() const;

  void addSubmodule(std::string name, std::string _filename, ModuleType type,
                    utils::VisibilityInfo visib_info);

  void closeSubmodule();

public:
  ~QatModule();

  static QatModule *Create(std::string name, fs::path filepath, ModuleType type,
                           utils::VisibilityInfo visib_info);

  std::string getFullName() const;

  std::string getName() const;

  std::string getFullNameWithChild(std::string name) const;

  QatModule *getActive();

  std::pair<unsigned, std::string>
  resolveNthParent(const std::string name) const;

  bool hasNthParent(unsigned int n) const;

  QatModule *getNthParent(unsigned int n);

  const utils::VisibilityInfo &getVisibility() const;

  Function *createFunction(const std::string name, QatType *returnType,
                           const bool isAsync, const std::vector<Argument> args,
                           const bool isVariadic, const bool returnsReference,
                           const utils::FilePlacement placement,
                           const utils::VisibilityInfo visibility);

  bool isSubmodule() const;

  // LIB

  bool hasLib(const std::string name) const;

  bool hasBroughtLib(const std::string name) const;

  std::pair<bool, std::string>
  hasAccessibleLibInImports(const std::string name,
                            const utils::RequesterInfo &reqInfo) const;

  QatModule *getLib(const std::string name,
                    const utils::RequesterInfo &reqInfo);

  void openLib(std::string name, std::string filename,
               utils::VisibilityInfo visib_info);

  void closeLib();

  // BOX

  bool hasBox(const std::string name) const;

  bool hasBroughtBox(const std::string name) const;

  QatModule *getBox(const std::string name,
                    const utils::RequesterInfo &reqInfo);

  std::pair<bool, std::string>
  hasAccessibleBoxInImports(const std::string name,
                            const utils::RequesterInfo &reqInfo) const;

  void openBox(std::string name,
               std::optional<utils::VisibilityInfo> visib_info);

  void closeBox();

  // FUNCTION

  bool hasFunction(const std::string name) const;

  bool hasBroughtFunction(const std::string name) const;

  Function *getFunction(const std::string name,
                        const utils::RequesterInfo &reqInfo);

  std::pair<bool, std::string>
  hasAccessibleFunctionInImports(const std::string name,
                                 const utils::RequesterInfo &reqInfo) const;

  // CORE TYPE

  bool hasCoreType(const std::string name) const;

  bool hasBroughtCoreType(const std::string name) const;

  std::pair<bool, std::string>
  hasAccessibleCoreTypeInImports(const std::string name,
                                 const utils::RequesterInfo &reqInfo) const;

  CoreType *getCoreType(const std::string name,
                        const utils::RequesterInfo &reqInfo) const;

  // GLOBAL ENTITY

  bool hasGlobalEntity(const std::string name) const;

  bool hasBroughtGlobalEntity(const std::string name) const;

  std::pair<bool, std::string>
  hasAccessibleGlobalEntityInImports(const std::string name,
                                     const utils::RequesterInfo &reqInfo) const;

  GlobalEntity *getGlobalEntity(const std::string name,
                                const utils::RequesterInfo &reqInfo) const;

  GlobalEntity *createGlobalEntity();

  // IMPORT

  bool hasImport(const std::string name) const;

  QatModule *getImport(const std::string name) const;

  std::pair<bool, std::string>
  hasAccessibleNamedImportInImports(const std::string name,
                                    const utils::RequesterInfo &reqInfo) const;

  // BRING ENTITIES

  void bring_module(QatModule *other, const utils::VisibilityInfo _visibility);

  void bring_named_module(const std::string _name, QatModule *other,
                          const utils::VisibilityInfo _visibility);

  void bring_entity(const std::string name,
                    const utils::VisibilityInfo _visibility);

  void bring_named_entity(const std::string name, const std::string entity,
                          const utils::VisibilityInfo _visibility);

  llvm::GlobalVariable *get_global_variable(std::string name,
                                            utils::RequesterInfo &req_info);

  void throw_error(std::string message, utils::FilePlacement placement);
};

} // namespace IR
} // namespace qat

#endif