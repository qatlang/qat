#ifndef QAT_AST_OR_USE_VALUE_HPP
#define QAT_AST_OR_USE_VALUE_HPP

#include "../expression.hpp"

namespace qat::ast {

class OrUseValue : public Expression {
	Expression* expression;
	Expression* candidate;

  public:
	OrUseValue(Expression* _expression, Expression* _candidate, FileRangePtr _fileRange)
	    : Expression(_fileRange), expression(_expression), candidate(_candidate) {}

	static OrUseValue* create(Expression* expression, Expression* candidate, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(OrUseValue), expression, candidate, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(expression);
		UPDATE_DEPS(candidate);
	}

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::OR_USE_VALUE; }
};

} // namespace qat::ast

#endif
