#ifndef QAT_AST_EXPRESSION_GET_POLY_HPP
#define QAT_AST_EXPRESSION_GET_POLY_HPP

#include "../expression.hpp"
#include "../skill_entity.hpp"
#include "../types/pointer.hpp"

namespace qat::ast {

class PolySkillSpec {
  public:
	std::variant<SkillEntity, DoneSkillEntity> spec;
	FileRange                                  range;

	PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity> _spec, FileRange _range)
	    : spec(std::move(_spec)), range(std::move(_range)) {}

	useit static PolySkillSpec from_skill(SkillEntity skill, FileRange range) {
		return PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity>{std::in_place_index<0>, skill},
		                     std::move(range));
	}

	useit static PolySkillSpec from_implementation(DoneSkillEntity done, FileRange range) {
		return PolySkillSpec(std::variant<SkillEntity, DoneSkillEntity>(std::in_place_index<1>, done),
		                     std::move(range));
	}

	useit bool is_skill() const { return spec.index() == 0u; }

	useit bool is_done_skill() const { return spec.index() == 1u; }

	useit SkillEntity const& as_skill() const { return std::get<0>(spec); }

	useit SkillEntity& as_skill() { return std::get<0>(spec); }

	useit DoneSkillEntity const& as_done_skill() const { return std::get<1>(spec); }

	useit DoneSkillEntity& as_done_skill() { return std::get<1>(spec); }

	useit FileRange get_range() const { return range; }

	useit Json to_json() const { return is_skill() ? as_skill().to_json() : as_done_skill().to_json(); }
};

class GetPolymorph final : public Expression {
	Expression*        value;
	bool               isVar;
	Maybe<FileRange>   isTypeRange;
	Vec<PolySkillSpec> skills;

	Maybe<PtrOwner> owner;

  public:
	GetPolymorph(Expression* _value, Maybe<FileRange> _isTypeRange, Vec<PolySkillSpec> _skills, Maybe<PtrOwner> _owner,
	             FileRange _fileRange)
	    : Expression(std::move(_fileRange)), value(_value), isTypeRange(std::move(_isTypeRange)),
	      skills(std::move(_skills)), owner(std::move(_owner)) {}

	useit static GetPolymorph* create(Expression* value, Maybe<FileRange> isTypeRange, Vec<PolySkillSpec> skills,
	                                  Maybe<PtrOwner> owner, FileRange fileRange) {
		return std::construct_at(OwnNormal(GetPolymorph), value, std::move(isTypeRange), std::move(skills), owner,
		                         std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto& sk : skills) {
			if (sk.is_skill()) {
				sk.as_skill().update_dependencies(phase, ir::DependType::complete, ent, ctx);
			} else {
				sk.as_done_skill().update_dependencies(phase, ir::DependType::partial, ent, ctx);
			}
		}
		if (owner.has_value() && owner.value().candidate) {
			owner.value().candidate->update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::GET_POLYMORPH; }

	useit Json to_json() const final {
		Vec<JsonValue> skillsJSON;
		for (auto& sk : skills) {
			skillsJSON.push_back(sk.to_json());
		}
		return Json()
		    ._("nodeType", "getPolymorph")
		    ._("value", value->to_json())
		    ._("isType", isTypeRange.has_value())
		    ._("typeRange", isTypeRange.has_value() ? (JsonValue)isTypeRange.value() : JsonValue())
		    ._("skillSpecifications", skillsJSON)
		    ._("hasPointerOwner", owner.has_value())
		    ._("pointerOwner", owner.has_value() ? owner.value().to_json() : JsonValue())
		    ._("fileRange", fileRange);
	}
};

} // namespace qat::ast

#endif
