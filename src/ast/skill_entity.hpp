#ifndef QAT_AST_SKILL_ENTITY_HPP
#define QAT_AST_SKILL_ENTITY_HPP

#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "./generics.hpp"

namespace qat::ast {

struct SkillEntity {
	u32               relative;
	Vec<Identifier>   names;
	FileRangePtr      range;
	Vec<FillGeneric*> generics;

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx);

	ir::Skill* find_skill(EmitCtx* ctx) const;

	String to_string() const;
};

struct DoneSkillEntity {
	u32               relative;
	Vec<Identifier>   names;
	FileRangePtr      range;
	Vec<FillGeneric*> generics;

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx);

	ir::DoneSkill* find_done_skill(EmitCtx* ctx) const;

	String to_string() const;
};

} // namespace qat::ast

#endif
