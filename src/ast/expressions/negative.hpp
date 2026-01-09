#ifndef QAT_AST_NEGATIVE_HPP
#define QAT_AST_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Negative final : public Expression, public TypeInferrable {
	Expression* value;

  public:
	Negative(Expression* _value, FileRangePtr _fileRange) : Expression(_fileRange), value(_value) {}

	useit static Negative* create(Expression* value, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Negative), value, fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(value);
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::NEGATIVE; }
};

} // namespace qat::ast

#endif
