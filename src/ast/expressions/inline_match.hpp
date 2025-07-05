#ifndef QAT_AST_EXPRESSIONS_INLINE_MATCH_HPP
#define QAT_AST_EXPRESSIONS_INLINE_MATCH_HPP

#include "../expression.hpp"

namespace qat::ast {

class InlineMatch final : public Expression, public TypeInferrable {
	Expression*      expression;
	Vec<Expression*> values;

  public:
	InlineMatch(Expression* _expression, Vec<Expression*> _values, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), expression(_expression), values(std::move(_values)) {}

	useit static InlineMatch* create(Expression* expression, Vec<Expression*> values, FileRange fileRange) {
		return std::construct_at(OwnNormal(InlineMatch), expression, std::move(values), std::move(fileRange));
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::INLINE_MATCH; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
