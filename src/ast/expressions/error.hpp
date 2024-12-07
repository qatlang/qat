#ifndef QAT_AST_EXPRESSIONS_ERROR_HPP
#define QAT_AST_EXPRESSIONS_ERROR_HPP

#include "../expression.hpp"

namespace qat::ast {

class ErrorExpression : public Expression, public LocalDeclCompatible, public InPlaceCreatable, public TypeInferrable {
  private:
	Expression*									   errorValue;
	Maybe<Pair<FileRange, ast::PrerunExpression*>> isPacked;
	Maybe<Pair<Type*, Type*>>					   providedType;

  public:
	ErrorExpression(Expression* _value, Maybe<Pair<FileRange, ast::PrerunExpression*>> _isPacked,
					Maybe<Pair<Type*, Type*>> _providedType, FileRange _fileRange)
		: Expression(_fileRange), errorValue(_value), isPacked(_isPacked), providedType(_providedType) {}

	useit static ErrorExpression* create(Expression* value, Maybe<Pair<FileRange, ast::PrerunExpression*>> isPacked,
										 Maybe<Pair<Type*, Type*>> providedType, FileRange fileRange) {
		return std::construct_at(OwnNormal(ErrorExpression), value, isPacked, providedType, fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS
	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit Json		 to_json() const final;
	useit NodeType	 nodeType() const final { return NodeType::ERROR_EXPRESSION; }
};

} // namespace qat::ast

#endif
