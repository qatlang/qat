#ifndef QAT_IR_TYPES_POLYMORPH_HPP
#define QAT_IR_TYPES_POLYMORPH_HPP

#include "../../utils/qat_region.hpp"
#include "./pointer.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Skill;

class Polymorph final : public Type {
	bool        isTyped;
	Vec<Skill*> skills;
	MarkOwner   owner;

  public:
	Polymorph(bool _isTyped, Vec<Skill*> _skills, MarkOwner _owner, ir::Ctx* ctx);

	useit static Polymorph* create(bool isTyped, Vec<Skill*> skills, MarkOwner owner, ir::Ctx* ctx) {
		return std::construct_at(OwnNormal(Polymorph), isTyped, std::move(skills), owner, ctx);
	}

	~Polymorph() = default;

	useit bool is_typed_poly() const { return isTyped; }

	useit Vec<Skill*> const& get_skills() const { return skills; }

	useit MarkOwner const& get_owner() const { return owner; }

	useit bool is_type_sized() const final { return true; }

	useit bool is_trivially_copyable() const final { return true; }

	useit bool is_trivially_movable() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::polymorph; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
