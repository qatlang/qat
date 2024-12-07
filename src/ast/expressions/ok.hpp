#ifndef QAT_AST_EXPRESSIONS_OK_HPP
#define QAT_AST_EXPRESSIONS_OK_HPP

#include "../expression.hpp"

namespace qat::ast {

class OkExpression final : public Expression,
						   public LocalDeclCompatible,
						   public InPlaceCreatable,
						   public TypeInferrable {
	Expression*								  subExpr;
	Maybe<Pair<FileRange, PrerunExpression*>> isPacked;
	Maybe<Pair<Type*, Type*>>				  providedType;

  public:
	OkExpression(Expression* _subExpr, Maybe<Pair<FileRange, PrerunExpression*>> _isPacked,
				 Maybe<Pair<Type*, Type*>> _providedType, FileRange _fileRange)
		: Expression(_fileRange), subExpr(_subExpr), isPacked(_isPacked), providedType(_providedType){};

	useit static OkExpression* create(Expression* subExpr, Maybe<Pair<FileRange, PrerunExpression*>> isPacked,
									  Maybe<Pair<Type*, Type*>> providedType, FileRange fileRange) {
		return std::construct_at(OwnNormal(OkExpression), subExpr, isPacked, providedType, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType	 nodeType() const final { return NodeType::OK; }
	useit Json		 to_json() const final;
};

} // namespace qat::ast

#endif
