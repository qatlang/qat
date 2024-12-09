#ifndef QAT_AST_PRERUN_FLOAT_LITERAL_HPP
#define QAT_AST_PRERUN_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FloatLiteral final : public PrerunExpression, public TypeInferrable {
  private:
	String value;

  public:
	FloatLiteral(String _value, FileRange _fileRange) : PrerunExpression(_fileRange), value(_value) {}

	useit static FloatLiteral* create(String _value, FileRange _fileRange) {
		return std::construct_at(OwnNormal(FloatLiteral), _value, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::PrerunValue* emit(EmitCtx* ctx) override;
	useit Json             to_json() const override;
	useit String           to_string() const override;
	useit NodeType         nodeType() const override { return NodeType::FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif
