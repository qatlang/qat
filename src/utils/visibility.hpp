#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include <map>
#include <nuo/json.hpp>
#include <optional>
#include <string>

namespace qat::utils {

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

  bool operator==(VisibilityInfo other) const;

  operator nuo::Json() const;

  operator nuo::JsonValue() const;

  VisibilityInfo(const VisibilityInfo &other);
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
};

class Visibility {
public:
  static const std::map<VisibilityKind, std::string> kind_value_map;

  static const std::map<std::string, VisibilityKind> value_kind_map;

  static std::string getValue(VisibilityKind kind);

  static VisibilityKind getKind(std::string value);

  static bool isAccessible(VisibilityInfo visibility, RequesterInfo reqInfo);
};

} // namespace qat::utils

#endif