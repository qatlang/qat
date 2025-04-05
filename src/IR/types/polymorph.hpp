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
	PtrOwner    owner;

  public:
	Polymorph(bool _isTyped, Vec<Skill*> _skills, PtrOwner _owner, ir::Ctx* ctx);

	useit static Polymorph* create(bool isTyped, Vec<Skill*> skills, PtrOwner owner, ir::Ctx* ctx) {
		return std::construct_at(OwnNormal(Polymorph), isTyped, std::move(skills), owner, ctx);
	}

	~Polymorph() = default;

	useit bool is_typed_poly() const { return isTyped; }

	useit Vec<Skill*> const& get_skills() const { return skills; }

	useit PtrOwner const& get_owner() const { return owner; }

	useit bool is_type_sized() const final { return true; }

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::POLYMORPH; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
