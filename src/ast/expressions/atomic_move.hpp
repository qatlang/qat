#ifndef QAT_AST_EXPRESSIONS_ATOMIC_MOVE_HPP
#define QAT_AST_EXPRESSIONS_ATOMIC_MOVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class AtomicMove : public Expression {
	Expression*       candidate;
	PrerunExpression* ordering;

  public:
	AtomicMove(Expression* _candidate, PrerunExpression* _ordering, FileRangePtr _fileRange)
	    : Expression(_fileRange), candidate(_candidate), ordering(_ordering) {}

	useit static AtomicMove* create(Expression* candidate, PrerunExpression* ordering, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(AtomicMove), candidate, ordering, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		if (ordering) {
			UPDATE_DEPS(ordering);
		}
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::ATOMIC_MOVE; }
};

} // namespace qat::ast

#endif
