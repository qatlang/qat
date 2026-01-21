#ifndef QAT_IR_TYPES_POLYMORPH_HPP
#define QAT_IR_TYPES_POLYMORPH_HPP

#include "./pointer.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Skill;

class Polymorph final : public Type {
	friend class Type;
	bool                isTyped;
	bool                isVar;
	Vec<Skill*>         skills;
	Maybe<Locality>     locality;
	Maybe<AddressSpace> addressSpace;

	static Vec<Polymorph*> allPolyTypes;

  public:
	Polymorph(bool _isTyped, bool _isVar, Vec<Skill*> _skills, Maybe<Locality> _locality, Maybe<AddressSpace>,
	          ir::Ctx* ctx);

	static Polymorph* create(bool isTyped, bool isVar, Vec<Skill*> skills, Maybe<Locality> locality,
	                         Maybe<AddressSpace> addressSpace, ir::Ctx* ctx);

	~Polymorph() = default;

	bool is_typed_poly() const { return isTyped; }

	bool has_variability() const { return isVar; }

	Vec<Skill*> const& get_skills() const { return skills; }

	bool has_locality() const { return locality.has_value(); }

	bool has_address_space() const { return addressSpace.has_value(); }

	Locality get_locality() const { return locality.value(); }

	Maybe<AddressSpace> const& get_address_space() const { return addressSpace; }

	bool has_skill(Skill* skill) const {
		for (auto sk : skills) {
			if (sk == skill) {
				return true;
			}
		}
		return false;
	}

	usize get_skill_index_in_type(Skill* skill) const {
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

	bool is_type_sized() const final { return true; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::POLYMORPH; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
