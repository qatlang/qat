#ifndef QAT_AST_PRERUN_EXPRESSIONS_SUB_ENTITY_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_SUB_ENTITY_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class PrerunSubEntity final : public PrerunExpression {
	Maybe<FileRangePtr> skill;
	Maybe<FileRangePtr> doneSkill;
	Vec<Identifier>     names;
	TypeLike            parentType;

  public:
	PrerunSubEntity(Maybe<FileRangePtr> _skill, Maybe<FileRangePtr> _doneSkill, Vec<Identifier> _names,
	                TypeLike _parentType, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	static PrerunSubEntity* create(Maybe<FileRangePtr> skill, Maybe<FileRangePtr> doneSkill, Vec<Identifier> names,
	                               TypeLike parentType, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunSubEntity), std::move(skill), std::move(doneSkill), std::move(names),
		                         parentType, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	ir::PrerunValue* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::PRERUN_SUB_ENTITY; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
