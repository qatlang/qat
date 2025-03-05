#ifndef QAT_AST_PRERUN_MIX_CHOICE_INITIALISER_HPP
#define QAT_AST_PRERUN_MIX_CHOICE_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunMixOrChoiceInit final : public PrerunExpression, public TypeInferrable {
	friend class LocalDeclaration;

	TypeLike                 type;
	Identifier               subName;
	Maybe<PrerunExpression*> expression;

  public:
	PrerunMixOrChoiceInit(TypeLike _type, Identifier _subName, Maybe<PrerunExpression*> _expression,
	                      FileRange _fileRange)
	    : PrerunExpression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

	useit static PrerunMixOrChoiceInit* create(TypeLike type, Identifier subName, Maybe<PrerunExpression*> expression,
	                                           FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunMixOrChoiceInit), type, subName, expression, fileRange);
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

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit Json to_json() const final;

	useit String to_string() const final;

	useit NodeType nodeType() const final { return NodeType::MIX_OR_CHOICE_INITIALISER; }
};

} // namespace qat::ast

#endif
