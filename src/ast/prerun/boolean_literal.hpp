#ifndef QAT_AST_PRERUN_BOOLEAN_LITERAL_HPP
#define QAT_AST_PRERUN_BOOLEAN_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class BooleanLiteral final : public PrerunExpression {
	bool value;

  public:
	BooleanLiteral(bool _value, FileRange _fileRange) : PrerunExpression(std::move(_fileRange)), value(_value) {}

	useit static BooleanLiteral* create(bool _value, FileRange _fileRange) {
		return std::construct_at(OwnNormal(BooleanLiteral), _value, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit Json             to_json() const final;
	useit String           to_string() const final;
	useit NodeType         nodeType() const final { return NodeType::BOOLEAN_LITERAL; }
};

} // namespace qat::ast

#endif
