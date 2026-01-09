#ifndef QAT_AST_EXPRESSIONS_SUB_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_SUB_ENTITY_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class SubEntity final : public Expression {
	Maybe<FileRangePtr> skill;
	Maybe<FileRangePtr> doneSkill;
	Vec<Identifier>     names;
	TypeLike            parentType;

  public:
	SubEntity(Maybe<FileRangePtr> _skill, Maybe<FileRangePtr> _doneSkill, Vec<Identifier> _names, TypeLike _parentType,
	          FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	static SubEntity* create(Maybe<FileRangePtr> skill, Maybe<FileRangePtr> doneSkill, Vec<Identifier> names,
	                         TypeLike parentType, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(SubEntity), std::move(skill), std::move(doneSkill), std::move(names),
		                         parentType, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::SUB_ENTITY; }
};

} // namespace qat::ast

#endif
