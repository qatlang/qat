#ifndef QAT_AST_PRERUN_NEGATIVE_HPP
#define QAT_AST_PRERUN_NEGATIVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunNegative final : public PrerunExpression, public TypeInferrable {
	PrerunExpression* value;

  public:
	PrerunNegative(PrerunExpression* _value, FileRangePtr _fileRange) : PrerunExpression(_fileRange), value(_value) {}

	static PrerunNegative* create(PrerunExpression* value, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunNegative), value, fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(value);
	}

	ir::PrerunValue* emit(EmitCtx* ctx);

	String to_string() const;

	NodeType nodeType() const { return NodeType::PRERUN_NEGATIVE; }
};

} // namespace qat::ast

#endif
