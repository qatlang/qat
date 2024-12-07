#ifndef QAT_AST_PRERUN_UNSIGNED_LITERAL_HPP
#define QAT_AST_PRERUN_UNSIGNED_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class UnsignedLiteral final : public PrerunExpression, public TypeInferrable {
  private:
	String						value;
	Maybe<Pair<u64, FileRange>> bits;

  public:
	UnsignedLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
		: PrerunExpression(_fileRange), value(_value), bits(_bits) {}

	useit static UnsignedLiteral* create(String _value, Maybe<Pair<u64, FileRange>> bits, FileRange _fileRange) {
		return std::construct_at(OwnNormal(UnsignedLiteral), _value, bits, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) override;
	useit Json			   to_json() const override;
	useit String		   to_string() const final;
	useit NodeType		   nodeType() const override { return NodeType::UNSIGNED_LITERAL; }
};

} // namespace qat::ast

#endif
