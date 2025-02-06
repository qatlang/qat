#ifndef QAT_AST_EXPRESSIONS_SUB_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_SUB_ENTITY_HPP

#include "../expression.hpp"

namespace qat::ast {

class SubEntity final : public Expression {
	Maybe<FileRange> skill;
	Maybe<FileRange> doneSkill;
	Vec<Identifier>  names;
	Type*            parentType;

  public:
	SubEntity(Maybe<FileRange> _skill, Maybe<FileRange> _doneSkill, Vec<Identifier> _names, Type* _parentType,
	          FileRange _fileRange)
	    : Expression(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	useit static SubEntity* create(Maybe<FileRange> skill, Maybe<FileRange> doneSkill, Vec<Identifier> names,
	                               Type* parentType, FileRange fileRange) {
		return std::construct_at(OwnNormal(SubEntity), std::move(skill), std::move(doneSkill), std::move(names),
		                         parentType, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::SUB_ENTITY; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
