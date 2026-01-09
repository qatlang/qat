#ifndef QAT_AST_EXPRESSIONS_LOGICAL_NOT_HPP
#define QAT_AST_EXPRESSIONS_LOGICAL_NOT_HPP

#include "../expression.hpp"

namespace qat::ast {

class LogicalNot final : public Expression {
	Expression* exp;

  public:
	LogicalNot(Expression* _exp, FileRangePtr _range) : Expression(_range), exp(_exp) {}

	static LogicalNot* create(Expression* _exp, FileRangePtr _range) {
		return std::construct_at(OwnNormal(LogicalNot), _exp, _range);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(exp);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::NOT; }
};

} // namespace qat::ast

#endif
