#ifndef QAT_AST_PRERUN_INTEGER_LITERAL_HPP
#define QAT_AST_PRERUN_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral final : public PrerunExpression, public TypeInferrable {
  private:
	String						value;
	Maybe<Pair<u64, FileRange>> bits;

  public:
	IntegerLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
		: PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bits(_bits) {}

	useit static IntegerLiteral* create(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange) {
		return std::construct_at(OwnNormal(IntegerLiteral), _value, _bits, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::PrerunValue* emit(EmitCtx* ctx) override;
	useit Json			   to_json() const override;
	useit String		   to_string() const override;
	useit NodeType		   nodeType() const override { return NodeType::INTEGER_LITERAL; }
};

} // namespace qat::ast

#endif
