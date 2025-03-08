#ifndef QAT_UTILS_VISIBILITY_HPP
#define QAT_UTILS_VISIBILITY_HPP

#include "./helpers.hpp"
#include "./json.hpp"
#include "./macros.hpp"

namespace qat {

namespace ir {
class Type;
class Mod;
class DoneSkill;
} // namespace ir

enum class VisibilityKind {
	type,
	pub,
	lib,
	file,
	folder,
	parent,
	skill,
};

String kind_to_string(VisibilityKind kind);

class AccessInfo;

class VisibilityInfo {
  private:
	VisibilityInfo(VisibilityKind _kind, ir::Mod* _module) : kind(_kind), moduleVal(_module) {}

	explicit VisibilityInfo(VisibilityKind _kind, ir::Type* _type) : kind(_kind), typePtr(_type) {}

  public:
	VisibilityKind kind;
	ir::Mod*       moduleVal = nullptr;
	ir::Type*      typePtr   = nullptr;

	VisibilityInfo(const VisibilityInfo& other);

	useit static VisibilityInfo type(ir::Type* type) { return VisibilityInfo(VisibilityKind::type, type); }

	useit static VisibilityInfo pub() { return {VisibilityKind::pub, (ir::Mod*)nullptr}; }

	useit static VisibilityInfo lib(ir::Mod* mod) { return {VisibilityKind::lib, mod}; }

	useit static VisibilityInfo file(ir::Mod* mod) { return {VisibilityKind::file, mod}; }

	useit static VisibilityInfo folder(ir::Mod* mod) { return {VisibilityKind::folder, mod}; }

	useit static VisibilityInfo skill(ir::Type* type) { return VisibilityInfo(VisibilityKind::skill, type); }

	useit bool is_accessible(Maybe<AccessInfo> reqInfo) const;

	useit bool operator==(const VisibilityInfo& other) const;

	useit operator Json() const;

	useit operator JsonValue() const;
};

class AccessInfo {
  private:
	ir::Mod*              module;
	Maybe<ir::Type*>      type;
	Maybe<ir::DoneSkill*> skill;

	bool isPrivileged = false;

  public:
	AccessInfo(ir::Mod* _lib, Maybe<ir::Type*> _type, Maybe<ir::DoneSkill*> _skill);

	useit static AccessInfo get_privileged();

	useit bool has_type() const { return type.has_value(); }

	useit bool has_skill() const { return skill.has_value(); }

	useit bool is_privileged_access() const;

	useit ir::Mod* get_module() const { return module; }

	useit ir::Type* get_type() const { return type.value(); }

	useit ir::DoneSkill* get_skill() const { return skill.value(); }
};

class Visibility {
  public:
	static const Map<VisibilityKind, String> kindValueMap;
	static const Map<String, VisibilityKind> valueKindMap;

	useit static String getValue(VisibilityKind kind);

	useit static VisibilityKind getKind(const String& value);

	useit static bool is_accessible(const VisibilityInfo& visibility, Maybe<AccessInfo> reqInfo);
};

} // namespace qat

#endif
