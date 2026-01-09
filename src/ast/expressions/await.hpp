#ifndef QAT_AST_EXPRESSIONS_AWAIT_HPP
#define QAT_AST_EXPRESSIONS_AWAIT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Await final : public Expression {
  private:
	Expression* exp;

  public:
	Await(Expression* _exp, FileRangePtr _fileRange) : Expression(_fileRange), exp(_exp) {}

	static Await* create(Expression* exp, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Await), exp, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(exp);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::AWAIT; }
};

} // namespace qat::ast

#endif
