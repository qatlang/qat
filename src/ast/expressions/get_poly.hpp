#ifndef QAT_AST_EXPRESSION_GET_POLY_HPP
#define QAT_AST_EXPRESSION_GET_POLY_HPP

#include "../expression.hpp"
#include "../skill_entity.hpp"
#include "../types/locality.hpp"
#include "../types/pointer.hpp"

#include <variant>

namespace qat::ast {

class PolySkillSpec {
  public:
	std::variant<SkillEntity, DoneSkillEntity> spec;
	FileRangePtr                               range;

	PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity> _spec, FileRangePtr _range)
	    : spec(std::move(_spec)), range(std::move(_range)) {}

	static PolySkillSpec from_skill(SkillEntity skill, FileRangePtr range) {
		return PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity>{std::in_place_index<0>, skill},
		                     std::move(range));
	}

	static PolySkillSpec from_implementation(DoneSkillEntity done, FileRangePtr range) {
		return PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity>(std::in_place_index<1>, done),
		                     std::move(range));
	}

	bool is_skill() const { return spec.index() == 0u; }

	bool is_done_skill() const { return spec.index() == 1u; }

	SkillEntity const& as_skill() const { return std::get<0>(spec); }

	SkillEntity& as_skill() { return std::get<0>(spec); }

	DoneSkillEntity const& as_done_skill() const { return std::get<1>(spec); }

	DoneSkillEntity& as_done_skill() { return std::get<1>(spec); }

	FileRangePtr get_range() const { return range; }
};

class GetPolymorph final : public Expression {
	Expression*         value;
	bool                isVar;
	Maybe<FileRangePtr> isTypeRange;
	Vec<PolySkillSpec>  skills;

	Maybe<Locality>     locality;
	Maybe<AddressSpace> addressSpace;

  public:
	GetPolymorph(Expression* _value, bool _isVar, Maybe<FileRangePtr> _isTypeRange, Vec<PolySkillSpec> _skills,
	             Maybe<Locality> _locality, Maybe<AddressSpace> _addressSpace, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), value(_value), isVar(_isVar), isTypeRange(std::move(_isTypeRange)),
	      skills(std::move(_skills)), locality(std::move(_locality)), addressSpace(std::move(_addressSpace)) {}

	static GetPolymorph* create(Expression* value, bool isVar, Maybe<FileRangePtr> isTypeRange,
	                            Vec<PolySkillSpec> skills, Maybe<Locality> locality, Maybe<AddressSpace> addressSpace,
	                            FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(GetPolymorph), value, isVar, std::move(isTypeRange), std::move(skills),
		                         std::move(locality), std::move(addressSpace), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto& sk : skills) {
			if (sk.is_skill()) {
				sk.as_skill().update_dependencies(phase, ir::DependType::complete, ent, ctx);
			} else {
				sk.as_done_skill().update_dependencies(phase, ir::DependType::partial, ent, ctx);
			}
		}
		if (locality.has_value() && locality.value().candidate) {
			locality.value().candidate->update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
		if (addressSpace.has_value() && addressSpace.value().value) {
			UPDATE_DEPS(addressSpace.value().value);
		}
	}

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::GET_POLYMORPH; }
};

} // namespace qat::ast

#endif
