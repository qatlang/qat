#ifndef QAT_AST_PRERUN_NEGATIVE_HPP
#define QAT_AST_PRERUN_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunNegative final : public PrerunExpression, public TypeInferrable {
	PrerunExpression* value;

  public:
	PrerunNegative(PrerunExpression* _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

	useit static PrerunNegative* create(PrerunExpression* value, FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunNegative), value, fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(value);
	}

	ir::PrerunValue* emit(EmitCtx* ctx);
	useit Json       to_json() const;
	useit String     to_string() const;
	useit NodeType   nodeType() const { return NodeType::PRERUN_NEGATIVE; }
};

} // namespace qat::ast

#endif
