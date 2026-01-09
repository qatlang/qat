#ifndef QAT_AST_PRERUN_FLOAT_LITERAL_HPP
#define QAT_AST_PRERUN_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class FloatLiteral final : public PrerunExpression, public TypeInferrable {
  private:
	String value;

  public:
	FloatLiteral(String _value, FileRangePtr _fileRange) : PrerunExpression(_fileRange), value(_value) {}

	static FloatLiteral* create(String _value, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(FloatLiteral), _value, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::PrerunValue* emit(EmitCtx* ctx) override;

	String to_string() const override;

	NodeType nodeType() const override { return NodeType::FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif
