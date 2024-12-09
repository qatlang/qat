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

  public:
	ArrayLiteral(Vec<Expression*> _values, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), values(std::move(_values)) {}

	useit static ArrayLiteral* create(Vec<Expression*> values, FileRange fileRange) {
		return std::construct_at(OwnNormal(ArrayLiteral), values, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto it : values) {
			UPDATE_DEPS(it);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit Json       to_json() const final;
	useit NodeType   nodeType() const final { return NodeType::ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif
