#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include "./helpers.hpp"
#include "./macros.hpp"
#include <map>
#include <nuo/json.hpp>

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
// TODO - Think about extension functions and how this should behave for those

// VisibilityKind tells where a type, global variable,
// function, library or box can be accessed
enum class VisibilityKind {
  type,   // Visibility is public only inside the parent type
  pub,    // Visibility is public, but attains the most strict parent
  lib,    // Visibility is public only inside the parent library
  file,   // Visibility is public only inside the parent file
  folder, // Visibility is public only inside the parent directory/folder
  box     // Visibility is public only inside the parent box
};

class RequesterInfo;

/**
 *  VisibilityInfo is used to store details about the visibility nature of
 * an entity. Some kinds require a value to be complete, others don't.
 *
 */
class VisibilityInfo {
private:
  VisibilityInfo(VisibilityKind _kind, String _value)
      : kind(_kind), value(std::move(_value)) {}

public:
  // Nature of the Visibility of the entity
  VisibilityKind kind;
  // Value related to the VisibilityKind. If not empty, this is either a
  // library name, box name or file path.
  String value;

  VisibilityInfo(const VisibilityInfo &other);

  static VisibilityInfo type(std::string typeName) {
    return {VisibilityKind::type, std::move(typeName)};
  }
  static VisibilityInfo pub() { return {VisibilityKind::pub, ""}; }
  static VisibilityInfo lib(String name) {
    return {VisibilityKind::lib, std::move(name)};
  }
  static VisibilityInfo file(String path) {
    return {VisibilityKind::file, std::move(path)};
  }
  static VisibilityInfo folder(String path) {
    return {VisibilityKind::folder, std::move(path)};
  }
  static VisibilityInfo box(String name) {
    return {VisibilityKind::box, std::move(name)};
  }

  useit bool isAccessible(const RequesterInfo &reqInfo) const;

  useit bool operator==(const VisibilityInfo &other) const;
             operator nuo::Json() const;
             operator nuo::JsonValue() const;
};

// Information about the entity requesting access
class RequesterInfo {
private:
  // Parent library of the entity requesting for access
  Maybe<String> lib;
  // Parent box of the entity requesting for access
  Maybe<String> box;
  // Compulsory parent file of the entity requesting for access
  String file;
  // Parent type of the entity requesting for access
  Maybe<String> type;

public:
  RequesterInfo(Maybe<String> _lib, Maybe<String> _box, String _file,
                Maybe<String> _type);

  useit bool   hasLib() const;
  useit bool   hasBox() const;
  useit bool   hasType() const;
  useit String getLib() const;
  useit String getBox() const;
  useit String getType() const;
  useit String getFile() const;
};

class Visibility {
public:
  static const std::map<VisibilityKind, String> kind_value_map;
  static const std::map<String, VisibilityKind> value_kind_map;

  useit static String         getValue(VisibilityKind kind);
  useit static VisibilityKind getKind(const String &value);
  useit static bool           isAccessible(const VisibilityInfo &visibility,
                                           const RequesterInfo  &reqInfo);
};

} // namespace qat::utils

#endif