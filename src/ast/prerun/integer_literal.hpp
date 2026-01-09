#ifndef QAT_AST_PRERUN_INTEGER_LITERAL_HPP
#define QAT_AST_PRERUN_INTEGER_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class IntegerLiteral final : public PrerunExpression, public TypeInferrable {
  private:
	String                         value;
	Maybe<Pair<u64, FileRangePtr>> bits;

  public:
	IntegerLiteral(String _value, Maybe<Pair<u64, FileRangePtr>> _bits, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bits(_bits) {}

	static IntegerLiteral* create(String _value, Maybe<Pair<u64, FileRangePtr>> _bits, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(IntegerLiteral), _value, _bits, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::PrerunValue* emit(EmitCtx* ctx) override;

	String to_string() const final;

	NodeType nodeType() const final { return NodeType::INTEGER_LITERAL; }
};

} // namespace qat::ast

#endif
