#ifndef QAT_AST_PRERUN_CHOICE_INITIALISER_HPP
#define QAT_AST_PRERUN_CHOICE_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class PrerunChoiceInitialiser : public TypeInferrable, public PrerunExpression {
	TypeLike   type;
	Identifier variant;

  public:
	PrerunChoiceInitialiser(TypeLike _type, Identifier _variant, FileRangePtr _fileRange)
	    : PrerunExpression(_fileRange), type(_type), variant(std::move(_variant)) {}

	static PrerunChoiceInitialiser* create(TypeLike type, Identifier variant, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunChoiceInitialiser), type, std::move(variant), fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		type.update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::PRERUN_CHOICE_INITIALISER; }

	String to_string() const final { return type.to_string() + "::" + variant.value; }
};

} // namespace qat::ast

#endif
