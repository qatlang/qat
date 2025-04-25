#ifndef QAT_AST_EXPRESSIONS_INLINE_LET_HPP
#define QAT_AST_EXPRESSIONS_INLINE_LET_HPP

#include "../expression.hpp"

namespace qat::ast {

class InlineLet final : public Expression {
	Expression* expression;

  public:
	InlineLet(Expression* _expression, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), expression(_expression) {}

	useit static InlineLet* create(Expression* expr, FileRange fileRange) {
		return std::construct_at(OwnNormal(InlineLet), expr, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::INLINE_LET; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
