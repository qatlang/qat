#ifndef QAT_AST_TYPES_POLYMORPH_HPP
#define QAT_AST_TYPES_POLYMORPH_HPP

#include "../skill_entity.hpp"
#include "./address_space.hpp"
#include "./pointer_owner.hpp"
#include "./qat_type.hpp"
#include "./type_kind.hpp"

namespace qat::ir {
class Skill;
}

namespace qat::ast {

class PolymorphType final : public Type {
	bool                isTyped;
	bool                isVar;
	Vec<SkillEntity>    skills;
	Maybe<Locality>     owner;
	Maybe<AddressSpace> addressSpace;

  public:
	PolymorphType(bool _isTyped, bool _isVar, Vec<SkillEntity> _skills, Maybe<Locality> _owner,
	              Maybe<AddressSpace> _addressSpace, FileRangePtr _range)
	    : Type(_range), isTyped(_isTyped), isVar(_isVar), skills(std::move(_skills)), owner(std::move(_owner)),
	      addressSpace(std::move(_addressSpace)) {}

	static PolymorphType* create(bool isTyped, bool isVar, Vec<SkillEntity> skills, Maybe<Locality> owner,
	                             Maybe<AddressSpace> addressSpace, FileRangePtr range) {
		return std::construct_at(OwnNormal(PolymorphType), isTyped, isVar, std::move(skills), std::move(owner),
		                         std::move(addressSpace), std::move(range));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto& sk : skills) {
			sk.update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
		if (owner.has_value() && owner.value().candidate) {
			owner.value().candidate->update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
		if (addressSpace.has_value() && addressSpace.value().value) {
			UPDATE_DEPS(addressSpace.value().value);
		}
	}

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const final { return AstTypeKind::POLYMORPH; }

	String to_string() const final {
		String skillStr;
		for (usize i = 0; i < skills.size(); i++) {
			skillStr += skills[i].to_string();
			if (i != (skills.size() - 1)) {
				skillStr += " + ";
			}
		}
		return String(isTyped ? (owner.has_value() ? "poly:ptr:[type, " : "poly:[type, ")
		                      : (owner.has_value() ? "poly:ptr[" : "poly:[")) +
		       (isVar ? "var " : "") + skillStr +
		       (owner.has_value() && owner.value().kind != LocalityKind::NONE ? (", " + owner.value().to_string())
		                                                                      : "") +
		       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
	}
};

} // namespace qat::ast

#endif
