#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include "./helpers.hpp"
#include "./json.hpp"
#include "./macros.hpp"
#include <map>

namespace qat {

namespace IR {
class QatType;
}

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
  box,    // Visibility is public only inside the parent box,
  parent
};

JsonValue kindToJsonValue(VisibilityKind kind);

class AccessInfo;

/**
 *  VisibilityInfo is used to store details about the visibility nature of
 * an entity. Some kinds require a value to be complete, others don't.
 *
 */
class VisibilityInfo {
private:
  VisibilityInfo(VisibilityKind _kind, String _value) : kind(_kind), value(std::move(_value)) {}
  explicit VisibilityInfo(IR::QatType* _type) : kind(VisibilityKind::type), typePtr(_type) {}

public:
  // Nature of the Visibility of the entity
  VisibilityKind kind;
  // Value related to the VisibilityKind. If not empty, this is either a
  // library name, box name or file path.
  String value;

  IR::QatType* typePtr = nullptr;

  VisibilityInfo(const VisibilityInfo& other);

  static VisibilityInfo type(IR::QatType* type) { return VisibilityInfo(type); }
  static VisibilityInfo pub() { return {VisibilityKind::pub, ""}; }
  static VisibilityInfo lib(String name) { return {VisibilityKind::lib, std::move(name)}; }
  static VisibilityInfo file(String path) { return {VisibilityKind::file, std::move(path)}; }
  static VisibilityInfo folder(String path) { return {VisibilityKind::folder, std::move(path)}; }
  static VisibilityInfo box(String name) { return {VisibilityKind::box, std::move(name)}; }

  useit bool isAccessible(const AccessInfo& reqInfo) const;

  useit bool operator==(const VisibilityInfo& other) const;
  operator Json() const;
  operator JsonValue() const;
};

// Information about the entity requesting access
class AccessInfo {
private:
  // Parent library of the entity requesting for access
  Maybe<String> lib;
  // Parent box of the entity requesting for access
  Maybe<String> box;
  // Compulsory parent file of the entity requesting for access
  String file;
  // Parent type of the entity requesting for access
  Maybe<IR::QatType*> type;

public:
  AccessInfo(Maybe<String> _lib, Maybe<String> _box, String _file, Maybe<IR::QatType*> _type);

  useit bool   hasLib() const;
  useit bool   hasBox() const;
  useit bool   hasType() const;
  useit String getLib() const;
  useit String getBox() const;
  useit IR::QatType* getType() const;
  useit String       getFile() const;
};

class Visibility {
public:
  static const std::map<VisibilityKind, String> kind_value_map;
  static const std::map<String, VisibilityKind> value_kind_map;

  useit static String         getValue(VisibilityKind kind);
  useit static VisibilityKind getKind(const String& value);
  useit static bool           isAccessible(const VisibilityInfo& visibility, const AccessInfo& reqInfo);
};

} // namespace qat

#endif