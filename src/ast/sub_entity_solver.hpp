#ifndef QAT_AST_SUB_ENTITY_SOLVER_HPP
#define QAT_AST_SUB_ENTITY_SOLVER_HPP

#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "../utils/macros.hpp"

namespace qat::ir {
class Value;
class Type;
class Skill;
class DoneSkill;
} // namespace qat::ir

namespace qat::ast {

struct EmitCtx;

enum class SubEntityParentKind {
	DONE_SKILL,
	TYPE,
	SKILL,
};

struct SubEntityParent {
	SubEntityParentKind kind;
	void*               data;
	FileRangePtr        range;

	static SubEntityParent of_type(ir::Type* type, FileRangePtr range) {
		return SubEntityParent{.kind = SubEntityParentKind::TYPE, .data = type, .range = range};
	}

	static SubEntityParent of_skill(ir::Skill* skill, FileRangePtr range) {
		return SubEntityParent{.kind = SubEntityParentKind::SKILL, .data = skill, .range = range};
	}

	static SubEntityParent of_done_skill(ir::DoneSkill* doneSkill, FileRangePtr range) {
		return SubEntityParent{.kind = SubEntityParentKind::DONE_SKILL, .data = doneSkill, .range = range};
	}
};

struct SubEntityResult {
	bool  isType;
	void* data;

	static SubEntityResult get_expression(ir::Value* value) {
		return SubEntityResult{
		    .isType = false,
		    .data   = (void*)value,
		};
	}

	static SubEntityResult get_type(ir::Type* type) {
		return SubEntityResult{
		    .isType = true,
		    .data   = (void*)type,
		};
	}
};

SubEntityResult sub_entity_solver(EmitCtx* ctx, bool isStrictlyPrerun, SubEntityParent currentParent,
                                  Vec<Identifier> const& names, FileRangePtr fileRange);

} // namespace qat::ast

#endif
