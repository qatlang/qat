#ifndef QAT_AST_SKILL_ENTITY_HPP
#define QAT_AST_SKILL_ENTITY_HPP

#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "./generics.hpp"

namespace qat::ast {

struct SkillEntity {
	u32               relative;
	Vec<Identifier>   names;
	FileRange         range;
	Vec<FillGeneric*> generics;

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx);

	useit ir::Skill* find_skill(EmitCtx* ctx) const;

	useit String to_string() const;

	useit Json to_json() const;
};

} // namespace qat::ast

#endif
