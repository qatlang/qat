#ifndef QAT_AST_ARRAY_LITERAL_HPP
#define QAT_AST_ARRAY_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunArrayLiteral final : public PrerunExpression, public TypeInferrable {
	Vec<PrerunExpression*> valuesExp;
	Type*                  elemTyHint;

  public:
	PrerunArrayLiteral(Vec<PrerunExpression*> _elements, Type* _elemTyHint, FileRangePtr _fileRange)
	    : PrerunExpression(_fileRange), valuesExp(_elements), elemTyHint(_elemTyHint) {}

	useit static PrerunArrayLiteral* create(Vec<PrerunExpression*> elements, Type* elemTyHint, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunArrayLiteral), elements, elemTyHint, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final;

	TYPE_INFERRABLE_FUNCTIONS

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit String to_string() const final;

	useit NodeType nodeType() const final { return NodeType::PRERUN_ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif
