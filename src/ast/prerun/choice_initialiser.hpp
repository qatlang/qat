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

	useit static PrerunChoiceInitialiser* create(TypeLike type, Identifier variant, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunChoiceInitialiser), type, std::move(variant), fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		type.update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::PRERUN_CHOICE_INITIALISER; }

	useit String to_string() const final { return type.to_string() + "::" + variant.value; }

	useit Json to_json() const final {
		return Json()
		    ._("hasType", (bool)type)
		    ._("type", type.to_json_value())
		    ._("variant", variant)
		    ._("fileRange", fileRange->to_json());
	}
};

} // namespace qat::ast

#endif
