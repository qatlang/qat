#ifndef QAT_AST_PRERUN_STRING_LITERAL_HPP
#define QAT_AST_PRERUN_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class StringLiteral final : public PrerunExpression {
	String value;

  public:
	StringLiteral(String _value, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

	static StringLiteral* create(String _value, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(StringLiteral), _value, _fileRange);
	}

	void addValue(const String& val, FileRangePtr fRange);

	String get_value() const;

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::PrerunValue* emit(EmitCtx* ctx) override;

	String to_string() const final;

	NodeType nodeType() const override { return NodeType::STRING_LITERAL; }
};

} // namespace qat::ast

#endif
