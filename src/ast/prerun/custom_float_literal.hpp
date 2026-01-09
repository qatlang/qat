#ifndef QAT_AST_PRERUN_CUSTOM_FLOAT_LITERAL_HPP
#define QAT_AST_PRERUN_CUSTOM_FLOAT_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class CustomFloatLiteral final : public PrerunExpression, public TypeInferrable {
	String value;
	String kind;

  public:
	CustomFloatLiteral(String _value, String _kind, FileRangePtr _fileRange)
	    : PrerunExpression(_fileRange), value(_value), kind(_kind) {}

	static CustomFloatLiteral* create(String _value, String _kind, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(CustomFloatLiteral), _value, _kind, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::PrerunValue* emit(EmitCtx* ctx) override;

	String to_string() const override;

	NodeType nodeType() const override { return NodeType::CUSTOM_FLOAT_LITERAL; }
};

} // namespace qat::ast

#endif
