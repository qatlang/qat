#ifndef QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class ArrayLiteral final : public Expression,
                           public LocalDeclCompatible,
                           public InPlaceCreatable,
                           public TypeInferrable {
	friend class LocalDeclaration;
	friend class Assignment;

  private:
	Vec<Expression*> values;
	Type*            elemTyHint;

  public:
	ArrayLiteral(Vec<Expression*> _values, Type* _elemTyHint, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), values(std::move(_values)), elemTyHint(_elemTyHint) {}

	useit static ArrayLiteral* create(Vec<Expression*> values, Type* elemTyHint, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(ArrayLiteral), values, elemTyHint, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif
