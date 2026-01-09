#ifndef QAT_AST_PRERUN_VARIANT_INITIALISER_HPP
#define QAT_AST_PRERUN_VARIANT_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class PrerunVariantInitialiser final : public PrerunExpression, public TypeInferrable {
	friend class LocalDeclaration;

	TypeLike                 type;
	Identifier               subName;
	Maybe<PrerunExpression*> expression;

  public:
	PrerunVariantInitialiser(TypeLike _type, Identifier _subName, Maybe<PrerunExpression*> _expression,
	                         FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

	static PrerunVariantInitialiser* create(TypeLike type, Identifier subName, Maybe<PrerunExpression*> expression,
	                                        FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunVariantInitialiser), type, subName, expression, fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type) {
			type.update_dependencies(phase, dep, ent, ctx);
		}
		if (expression.has_value()) {
			UPDATE_DEPS(expression.value());
		}
	}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	String to_string() const final;

	NodeType nodeType() const final { return NodeType::PRERUN_VARIANT_INITIALISER; }
};

} // namespace qat::ast

#endif
