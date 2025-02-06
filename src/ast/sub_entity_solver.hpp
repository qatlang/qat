#ifndef QAT_AST_SUB_ENTITY_SOLVER_HPP
#define QAT_AST_SUB_ENTITY_SOLVER_HPP

#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"

namespace qat::ir {
class Value;
class Type;
class Skill;
class DoneSkill;
} // namespace qat::ir

namespace qat::ast {

class EmitCtx;

enum class SubEntityParentKind {
	DONE_SKILL,
	TYPE,
	SKILL,
};

struct SubEntityParent {
	SubEntityParentKind kind;
	void*               data;
	FileRange           range;

	useit static SubEntityParent of_type(ir::Type* type, FileRange range) {
		return SubEntityParent{.kind = SubEntityParentKind::TYPE, .data = type, .range = range};
	}

	useit static SubEntityParent of_skill(ir::Skill* skill, FileRange range) {
		return SubEntityParent{.kind = SubEntityParentKind::SKILL, .data = skill, .range = range};
	}

	useit static SubEntityParent of_done_skill(ir::DoneSkill* doneSkill, FileRange range) {
		return SubEntityParent{.kind = SubEntityParentKind::DONE_SKILL, .data = doneSkill, .range = range};
	}
};

struct SubEntityResult {
	bool  isType;
	void* data;

	useit static SubEntityResult get_expression(ir::Value* value) {
		return SubEntityResult{
		    .isType = false,
		    .data   = (void*)value,
		};
	}

	useit static SubEntityResult get_type(ir::Type* type) {
		return SubEntityResult{
		    .isType = true,
		    .data   = (void*)type,
		};
	}
};

SubEntityResult sub_entity_solver(EmitCtx* ctx, bool isStrictlyPrerun, SubEntityParent currentParent,
                                  Vec<Identifier> const& names, FileRange fileRange);

} // namespace qat::ast

#endif
