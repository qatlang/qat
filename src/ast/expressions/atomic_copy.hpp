#ifndef QAT_AST_EXPRESSIONS_ATOMIC_COPY_HPP
#define QAT_AST_EXPRESSIONS_ATOMIC_COPY_HPP

#include "../expression.hpp"

namespace qat::ast {

class AtomicCopy : public Expression {
	Expression*       candidate;
	PrerunExpression* ordering;

  public:
	AtomicCopy(Expression* _candidate, PrerunExpression* _ordering, FileRangePtr _fileRange)
	    : Expression(_fileRange), candidate(_candidate), ordering(_ordering) {}

	static AtomicCopy* create(Expression* candidate, PrerunExpression* ordering, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(AtomicCopy), candidate, ordering, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		if (ordering) {
			UPDATE_DEPS(ordering);
		}
	}

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::ATOMIC_COPY; }
};

} // namespace qat::ast

#endif
