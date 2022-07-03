#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include <map>
#include <optional>

namespace qat {
namespace utils {

/**
 *  Visibility
 *
 * Libraries, Boxes, Types, Global Variables, Global & Static Functions
 * will always have a visibility associated with them.
 *
 * Visibility for Libraries, Global Variables and Functions are stored directly
 * in LLVM IR using metadata. Since boxes and types are constructs of the
 * language, those have to be handled separately.
 *
 *
 *
 */

/**
 *  VisibilityKind tells where a type, global variable,
 * function, library or box can be accessed
 *
 */
enum class VisibilityKind {
  // TODO - Think about extension functions and how this should behave for those
  /**
   *  Visibility is public only inside the parent type
   *
   */
  type,
  /**
   *  Visibility is public, but will attain the most strict visibility of
   * any of its parent entities
   *
   */
  pub,
  /**
   *  Visibility is public only inside the parent library
   *
   */
  lib,
  /**
   *  Visibility is public only inside the parent file
   *
   */
  file,
  /**
   *  Visibility is public only inside the parent directory/folder
   *
   */
  folder,
  /**
   *  Visibility is public only inside the parent box
   *
   */
  box
};

class RequesterInfo;

/**
 *  VisibilityInfo is used to store details about the visibility kind of
 * an entity. Some kinds require a value to be complete, others don't.
 *
 */
class VisibilityInfo {
private:
  VisibilityInfo(VisibilityKind _kind, std::string _value)
      : kind(_kind), value(_value) {}

public:
  /**
   *  VisibilityInfo with Private kind
   *
   * @return VisibilityInfo
   */
  static VisibilityInfo type() {
    return VisibilityInfo(VisibilityKind::type, "");
  }

  /**
   *  VisibilityInfo with Public kind
   *
   * @return VisibilityInfo
   */
  static VisibilityInfo pub() {
    return VisibilityInfo(VisibilityKind::pub, "");
  }

  /**
   *  VisibilityInfo with Library kind
   *
   * @param name Name of the library
   * @return VisibilityInfo
   */
  static VisibilityInfo lib(std::string name) {
    return VisibilityInfo(VisibilityKind::lib, name);
  }

  /**
   *  VisibilityInfo with File kind
   *
   * @param path Absolute path to the file
   * @return VisibilityInfo
   */
  static VisibilityInfo file(std::string path) {
    return VisibilityInfo(VisibilityKind::file, path);
  }

  /**
   *  VisibilityInfo
   *
   * @param path
   * @return VisibilityInfo
   */
  static VisibilityInfo folder(std::string path) {
    return VisibilityInfo(VisibilityKind::folder, path);
  }

  /**
   *  VisibilityInfo with Box kind
   *
   * @param name Name of the box
   * @return VisibilityInfo
   */
  static VisibilityInfo box(std::string name) {
    return VisibilityInfo(VisibilityKind::box, name);
  }

  bool isAccessible(const RequesterInfo &reqInfo) const;

  /**
   *  Kind of the Visibility of the entity
   *
   */
  VisibilityKind kind;

  /**
   *  Value related to the VisibilityKind. If not empty, this is either a
   * library name, box name or file path.
   *
   */
  std::string value;
};

/**
 *  Information about the entity requesting access
 *
 */
class RequesterInfo {
public:
  /**
   *  Parent library of the entity requesting for access.
   *
   */
  std::optional<std::string> lib;

  /**
   *  Parent box of the entity requesting for access.
   *
   */
  std::optional<std::string> box;

  /**
   *  Parent file of the entity requesting for access. This will never be
   * a null-value. This is also used for the parent folder
   *
   */
  std::string file;

  /**
   *  Parent type of the entity requesting for access.
   *
   */
  std::optional<std::string> type;

  /**
   *  Whether the requester has access to the lib represented by the
   * provided name
   *
   * @param other The lib to check for
   * @return true
   * @return false
   */
  bool has_access_to_lib(const std::string other) const;

  /**
   *  Whether the requester has access to the box represented by the
   * provided name
   *
   * @param other The box to check for
   * @return true
   * @return false
   */
  bool has_access_to_box(const std::string other) const;

  /**
   *  Whether the requester's parent file is the same as that of the
   * entity to be accessed
   *
   * @param other File path of the entity to check for
   * @return true
   * @return false
   */
  bool is_same_file(const std::string other) const;

  /**
   *  Whether the requester's parent type is the same as that of the
   * entity to be accessed
   *
   * @param other Name of the type of the entity to check for
   * @return true
   * @return false
   */
  bool is_same_type(const std::string other) const;
};

/**
 *  Visibility provides static functions to manage, set and retrieve
 * access kinds and info related to that
 *
 */
class Visibility {
private:
  /**
   *  VisibilityKind, VisibilityValue
   *
   */
  static const std::map<VisibilityKind, std::string> kind_value_map;

  /**
   *  VisibilityValue, VisibilityKind
   *
   */
  static const std::map<std::string, VisibilityKind> value_kind_map;

public:
  /**
   *  Get the appropriate linkage for an entity from its VisibilityKind
   *
   * @param kind VisibilityKind value
   * @return llvm::GlobalValue::LinkageTypes
   */
  static llvm::GlobalValue::LinkageTypes get_linkage(const VisibilityKind kind);

  /**
   *  Get the LLVM VisibilityTypes value of an entity from its
   * VisibilityKind
   *
   * @param kind VisibilityKind value
   * @return llvm::GlobalValue::VisibilityTypes
   */
  static llvm::GlobalValue::VisibilityTypes
  get_llvm_visibility(const VisibilityKind kind);

  /**
   *  Get the VisibilityKind of the provided GlobalVariable
   *
   * @param var The GlobalVariable to find the value of
   * @return VisibilityKind
   */
  static VisibilityKind get_kind(llvm::GlobalVariable *var);

  /**
   *  Get the VisibilityKind of the provided Function
   *
   * @param func The Function to find the value of
   * @return VisibilityKind
   */
  static VisibilityKind get_kind(llvm::Function *func);

  /**
   *  Get the VisibilityKind value of the provided Module
   *
   * @param mod The GlobalVariable to find the value of
   * @return VisibilityKind
   */
  static VisibilityKind get_kind(llvm::Module *mod);

  /**
   *  Get the VisibilityKind value of an entity with the provided name. It
   * can be a lib, box or type
   *
   * @param name The Name of the entity to find the value of
   * @return VisibilityKind
   */
  static VisibilityKind get_kind(std::string name);

  /**
   *  Get the visibility value of the provided GlobalVariable
   *
   * @param var
   * @return std::string
   */
  static std::string get_value(llvm::GlobalVariable *var);

  /**
   *  Get the visibility value of the provided Function
   *
   * @param func
   * @return std::string
   */
  static std::string get_value(llvm::Function *func);

  /**
   *  Get the visibility value of the provided Module
   *
   * @param mod
   * @return std::string
   */
  static std::string get_value(llvm::Module *mod);

  /**
   *  Get the visibility value of an entity with the provided name. It
   * can be a lib, box or type
   *
   * @param name The Name of the entity to find the value of
   * @return std::string
   */
  static std::string get_value(std::string name);

  static std::string get_value(VisibilityKind kind);

  /**
   *  Set the visibility of the provided GlobalVariable
   *
   * @param var
   * @param info
   * @param context
   */
  static void set(llvm::GlobalVariable *var, const VisibilityInfo info,
                  llvm::LLVMContext &context);

  static void set(llvm::Function *func, const VisibilityInfo info,
                  llvm::LLVMContext &context);

  static void set(llvm::Module *mod, const VisibilityInfo info,
                  llvm::LLVMContext &context);

  static void set(std::string name, std::string type, const VisibilityInfo info,
                  llvm::LLVMContext &context);

  /**
   *  Compare the provided visibility and requester-info and see if the
   * entity whose visibility is provided, can be accessed by the requester
   *
   * @param vinfo VisibilityInfo of the entity that has to be accessed
   * @param reqinfo RequesterInfo of the part that is trying to access the value
   * @return true If it is accessible
   * @return false If it is not accessible
   */
  static bool isAccessible(const VisibilityInfo vinfo,
                           const RequesterInfo reqinfo);
};

} // namespace utils
} // namespace qat

#endif