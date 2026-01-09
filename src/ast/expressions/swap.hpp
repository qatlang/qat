#ifndef QAT_AST_EXPRESSIONS_SWAP_HPP
#define QAT_AST_EXPRESSIONS_SWAP_HPP

#include "../expression.hpp"

namespace qat::ast {

class Swap final : public Expression {
  private:
	Expression* candidate;
	Expression* value;
	bool        isSelf;

  public:
	Swap(Expression* _candidate, Expression* _value, bool _isSelf, FileRangePtr _fileRange)
	    : Expression(_fileRange), candidate(_candidate), value(_value), isSelf(_isSelf) {}

	static Swap* create(Expression* candidate, Expression* value, bool isSelf, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Swap), candidate, value, isSelf, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		UPDATE_DEPS(value);
	}

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::SWAP; }
};

} // namespace qat::ast

#endif
