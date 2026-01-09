#ifndef QAT_AST_EXPRESSIONS_VARIANT_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_VARIANT_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class VariantInitialiser final : public Expression,
                                 public LocalDeclCompatible,
                                 public InPlaceCreatable,
                                 public TypeInferrable {
	friend class LocalDeclaration;

  private:
	TypeLike    type;
	Identifier  subName;
	Expression* expression;

  public:
	VariantInitialiser(TypeLike _type, Identifier _subName, Expression* _expression, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

	useit static VariantInitialiser* create(TypeLike type, Identifier subName, Expression* expression,
	                                        FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(VariantInitialiser), type, subName, expression, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		type.update_dependencies(phase, dep, ent, ctx);
		if (expression) {
			UPDATE_DEPS(expression);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::VARIANT_INITIALISER; }
};

} // namespace qat::ast

#endif
