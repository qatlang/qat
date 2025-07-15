#ifndef QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP
#define QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP

#include "../../IR/emit_phase.hpp"
#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class ConstructorCall final : public Expression,
                              public LocalDeclCompatible,
                              public InPlaceCreatable,
                              public TypeInferrable {
	friend class LocalDeclaration;

  private:
	TypeLike         type;
	Vec<Expression*> args;

  public:
	ConstructorCall(TypeLike _type, Vec<Expression*> _args, FileRangePtr _fileRange)
	    : Expression(_fileRange), type(_type), args(_args) {}

	useit static ConstructorCall* create(TypeLike _type, Vec<Expression*> _args, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ConstructorCall), _type, _args, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		type.update_dependencies(phase, ir::DependType::childrenPartial, ent, ctx);
		for (auto arg : args) {
			UPDATE_DEPS(arg);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::CONSTRUCTOR_CALL; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
