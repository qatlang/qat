#ifndef QAT_AST_PRERUN_TUPLE_VALUE_HPP
#define QAT_AST_PRERUN_TUPLE_VALUE_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunTupleValue final : public PrerunExpression, public TypeInferrable {
	Vec<PrerunExpression*> members;

  public:
	PrerunTupleValue(Vec<PrerunExpression*> _members, FileRange _fileRange)
	    : PrerunExpression(_fileRange), members(_members) {}

	useit static PrerunTupleValue* create(Vec<PrerunExpression*> _members, FileRange _fileRange) {
		return std::construct_at(OwnNormal(PrerunTupleValue), _members, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit Json             to_json() const final;
	useit String           to_string() const final;
	useit NodeType         nodeType() const final { return NodeType::PRERUN_TUPLE_VALUE; }
};

} // namespace qat::ast

#endif
