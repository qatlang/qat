#ifndef QAT_AST_EXPRESSIONS_NON_NULL_HPP
#define QAT_AST_EXPRESSIONS_NON_NULL_HPP

#include "../expression.hpp"

namespace qat::ast {

class NonNull : public Expression {
	Expression* candidate;

  public:
	NonNull(Expression* _candidate, FileRangePtr _fileRange) : Expression(_fileRange), candidate(_candidate) {}

	useit static NonNull* create(Expression* candidate, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(NonNull), candidate, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::NON_NULL; }
};

}; // namespace qat::ast

#endif
