#ifndef QAT_AST_PRERUN_DEFAULT_HPP
#define QAT_AST_PRERUN_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunDefault final : public PrerunExpression, public TypeInferrable {
	mutable Maybe<ast::Type*>				 theType;
	mutable Maybe<ast::GenericAbstractType*> genericAbstractType;

  public:
	PrerunDefault(Maybe<ast::Type*> _type, FileRange range) : PrerunExpression(range), theType(_type) {}

	useit static PrerunDefault* create(Maybe<ast::Type*> _type, FileRange _range) {
		return std::construct_at(OwnNormal(PrerunDefault), _type, _range);
	}

	void setGenericAbstract(ast::GenericAbstractType* genAbs) const;

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (theType.has_value()) {
			UPDATE_DEPS(theType.value());
		}
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit NodeType		   nodeType() const final { return NodeType::PRERUN_DEFAULT; }
	useit String		   to_string() const final;
	useit Json			   to_json() const final;
};

} // namespace qat::ast

#endif
