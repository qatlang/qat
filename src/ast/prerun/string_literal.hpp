#ifndef QAT_AST_PRERUN_STRING_LITERAL_HPP
#define QAT_AST_PRERUN_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class StringLiteral final : public PrerunExpression {
	String value;

  public:
	StringLiteral(String _value, FileRange _fileRange)
	    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

	useit static StringLiteral* create(String _value, FileRange _fileRange) {
		return std::construct_at(OwnNormal(StringLiteral), _value, _fileRange);
	}

	void addValue(const String& val, const FileRange& fRange);

	useit String get_value() const;

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) override;
	useit Json             to_json() const override;
	useit String           to_string() const final;
	useit NodeType         nodeType() const override { return NodeType::STRING_LITERAL; }
};

} // namespace qat::ast

#endif
