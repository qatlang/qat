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
class DoneSkill;
} // namespace IR

enum class VisibilityKind {
  type,
  pub,
  lib,
  file,
  folder,
  box,
  parent,
  skill,
};

JsonValue kindToJsonValue(VisibilityKind kind);

class AccessInfo;

class VisibilityInfo {
private:
  VisibilityInfo(VisibilityKind _kind, IR::QatModule* _module) : kind(_kind), moduleVal(_module) {}
  explicit VisibilityInfo(VisibilityKind _kind, IR::QatType* _type) : kind(_kind), typePtr(_type) {}

public:
  VisibilityKind kind;
  IR::QatModule* moduleVal = nullptr;
  IR::QatType*   typePtr   = nullptr;

  VisibilityInfo(const VisibilityInfo& other);

  static VisibilityInfo type(IR::QatType* type) { return VisibilityInfo(VisibilityKind::type, type); }
  static VisibilityInfo pub() { return {VisibilityKind::pub, (IR::QatModule*)nullptr}; }
  static VisibilityInfo lib(IR::QatModule* mod) { return {VisibilityKind::lib, mod}; }
  static VisibilityInfo file(IR::QatModule* mod) { return {VisibilityKind::file, mod}; }
  static VisibilityInfo folder(IR::QatModule* mod) { return {VisibilityKind::folder, mod}; }
  static VisibilityInfo box(IR::QatModule* mod) { return {VisibilityKind::box, mod}; }
  static VisibilityInfo skill(IR::QatType* type) { return VisibilityInfo(VisibilityKind::skill, type); }

  useit bool isAccessible(Maybe<AccessInfo> reqInfo) const;

  useit bool operator==(const VisibilityInfo& other) const;
  operator Json() const;
  operator JsonValue() const;
};

class AccessInfo {
private:
  IR::QatModule*        module;
  Maybe<IR::QatType*>   type;
  Maybe<IR::DoneSkill*> skill;

  bool isPrivileged = false;

public:
  AccessInfo(IR::QatModule* _lib, Maybe<IR::QatType*> _type, Maybe<IR::DoneSkill*> _skill);

  useit static AccessInfo GetPrivileged();

  useit bool        has_type() const { return type.has_value() && (type.value() != nullptr); }
  useit inline bool has_skill() const { return skill.has_value() && (skill.value() != nullptr); }
  useit bool        isPrivilegedAccess() const;
  useit IR::QatModule* getModule() const { return module; }
  useit IR::QatType*          get_type() const { return type.value(); }
  useit inline IR::DoneSkill* get_skill() const { return skill.value(); }
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