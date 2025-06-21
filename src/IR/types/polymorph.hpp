#ifndef QAT_IR_TYPES_POLYMORPH_HPP
#define QAT_IR_TYPES_POLYMORPH_HPP

#include "../../utils/qat_region.hpp"
#include "./pointer.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Skill;

class Polymorph final : public Type {
	friend class Type;
	bool            isTyped;
	bool            isVar;
	Vec<Skill*>     skills;
	Maybe<PtrOwner> owner;

  public:
	Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, Maybe<PtrOwner> _owner, ir::Ctx* ctx);

	useit static Polymorph* create(bool isTyped, bool isVar, Vec<Skill*> skills, Maybe<PtrOwner> owner, ir::Ctx* ctx);

	~Polymorph() = default;

	useit bool is_typed_poly() const { return isTyped; }

	useit bool has_variability() const { return isVar; }

	useit Vec<Skill*> const& get_skills() const { return skills; }

	useit bool has_owner() const { return owner.has_value(); }

	useit PtrOwner get_owner() const { return owner.value(); }

	useit bool has_skill(Skill* skill) const {
		for (auto sk : skills) {
			if (sk == skill) {
				return true;
			}
		}
		return false;
	}

	useit usize get_skill_index_in_type(Skill* skill) const {
		u8 offset = 1;
		if (isTyped) {
			offset += 1;
		}
		for (usize i = 0; i < skills.size(); i++) {
			if (skills[i] == skill) {
				return i + offset;
			}
		}
		return 0;
	}

	useit bool is_type_sized() const final { return true; }

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::POLYMORPH; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
