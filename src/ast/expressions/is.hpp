#ifndef QAT_AST_EXPRESSIONS_IS_HPP
#define QAT_AST_EXPRESSIONS_IS_HPP

#include "../expression.hpp"

namespace qat::ast {

class IsExpression final : public Expression,
                           public LocalDeclCompatible,
                           public InPlaceCreatable,
                           public TypeInferrable {
	friend class Assignment;
	friend class LocalDeclaration;

	Expression* subExpr = nullptr;

  public:
	IsExpression(Expression* _subExpr, FileRangePtr _fileRange) : Expression(_fileRange), subExpr(_subExpr) {}

	useit static IsExpression* create(Expression* subExpr, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(IsExpression), subExpr, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(subExpr);
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::IS; }
};

} // namespace qat::ast

#endif
