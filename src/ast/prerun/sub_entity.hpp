#ifndef QAT_AST_PRERUN_EXPRESSIONS_SUB_ENTITY_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_SUB_ENTITY_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunSubEntity final : public PrerunExpression {
	Maybe<FileRange>  skill;
	Maybe<FileRange>  doneSkill;
	Vec<Identifier>   names;
	Type*             parentType;
	PrerunExpression* parentExpression;

  public:
	PrerunSubEntity(Maybe<FileRange> _skill, Maybe<FileRange> _doneSkill, Vec<Identifier> _names, Type* _parentType,
	                PrerunExpression* _parentExpression, FileRange _fileRange)
	    : PrerunExpression(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	useit static PrerunSubEntity* create(Maybe<FileRange> skill, Maybe<FileRange> doneSkill, Vec<Identifier> names,
	                                     Type* parentType, PrerunExpression* parentExpression, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunSubEntity), std::move(skill), std::move(doneSkill), std::move(names),
		                         parentType, parentExpression, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::PrerunValue* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::PRERUN_SUB_ENTITY; }

	useit Json to_json() const final;

	useit String to_string() const final;
};

} // namespace qat::ast

#endif
