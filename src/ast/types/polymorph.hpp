#ifndef QAT_AST_TYPES_POLYMORPH_HPP
#define QAT_AST_TYPES_POLYMORPH_HPP

#include "../skill_entity.hpp"
#include "./address_space.hpp"
#include "./locality.hpp"
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
	Maybe<Locality>     locality;
	Maybe<AddressSpace> addressSpace;

  public:
	PolymorphType(bool _isTyped, bool _isVar, Vec<SkillEntity> _skills, Maybe<Locality> _locality,
	              Maybe<AddressSpace> _addressSpace, FileRangePtr _range)
	    : Type(_range), isTyped(_isTyped), isVar(_isVar), skills(std::move(_skills)), locality(std::move(_locality)),
	      addressSpace(std::move(_addressSpace)) {}

	static PolymorphType* create(bool isTyped, bool isVar, Vec<SkillEntity> skills, Maybe<Locality> locality,
	                             Maybe<AddressSpace> addressSpace, FileRangePtr range) {
		return std::construct_at(OwnNormal(PolymorphType), isTyped, isVar, std::move(skills), std::move(locality),
		                         std::move(addressSpace), std::move(range));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto& sk : skills) {
			sk.update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
		if (locality.has_value() && locality.value().candidate) {
			locality.value().candidate->update_dependencies(phase, ir::DependType::complete, ent, ctx);
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
		return String(isTyped ? (locality.has_value() ? "poly:ptr:[type, " : "poly:[type, ")
		                      : (locality.has_value() ? "poly:ptr[" : "poly:[")) +
		       (isVar ? "var " : "") + skillStr +
		       (locality.has_value() && locality.value().kind != LocalityKind::NONE
		            ? (", " + locality.value().to_string())
		            : "") +
		       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
	}
};

} // namespace qat::ast

#endif
