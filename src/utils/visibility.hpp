#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include "./helpers.hpp"
#include "./json.hpp"
#include "./macros.hpp"
#include <map>

namespace qat {

namespace IR {
class QatType;
class QatModule;
} // namespace IR

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
  VisibilityInfo(VisibilityKind _kind, IR::QatModule* _module) : kind(_kind), moduleVal(_module) {}
  explicit VisibilityInfo(IR::QatType* _type) : kind(VisibilityKind::type), typePtr(_type) {}

public:
  VisibilityKind kind;
  IR::QatModule* moduleVal = nullptr;
  IR::QatType*   typePtr   = nullptr;

  VisibilityInfo(const VisibilityInfo& other);

  static VisibilityInfo type(IR::QatType* type) { return VisibilityInfo(type); }
  static VisibilityInfo pub() { return {VisibilityKind::pub, nullptr}; }
  static VisibilityInfo lib(IR::QatModule* mod) { return {VisibilityKind::lib, mod}; }
  static VisibilityInfo file(IR::QatModule* mod) { return {VisibilityKind::file, mod}; }
  static VisibilityInfo folder(IR::QatModule* mod) { return {VisibilityKind::folder, mod}; }
  static VisibilityInfo box(IR::QatModule* mod) { return {VisibilityKind::box, mod}; }

  useit bool isAccessible(Maybe<AccessInfo> reqInfo) const;

  useit bool operator==(const VisibilityInfo& other) const;
  operator Json() const;
  operator JsonValue() const;
};

// Information about the entity requesting access
class AccessInfo {
private:
  IR::QatModule* module;
  // Parent type of the entity requesting for access
  Maybe<IR::QatType*> type;

  bool isPrivileged = false;

public:
  AccessInfo(IR::QatModule* _lib, Maybe<IR::QatType*> _type);

  useit static AccessInfo GetPrivileged();

  useit bool hasType() const;
  useit bool isPrivilegedAccess() const;
  useit IR::QatModule* getModule() const;
  useit IR::QatType* getType() const;
};

class Visibility {
public:
  static const std::map<VisibilityKind, String> kind_value_map;
  static const std::map<String, VisibilityKind> value_kind_map;

  useit static String         getValue(VisibilityKind kind);
  useit static VisibilityKind getKind(const String& value);
  useit static bool           isAccessible(const VisibilityInfo& visibility, Maybe<AccessInfo> reqInfo);
};

} // namespace qat

#endif