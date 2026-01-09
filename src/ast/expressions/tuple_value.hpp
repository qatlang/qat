#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"

#include <vector>

namespace qat::ast {

class TupleValue final : public Expression, public LocalDeclCompatible, public InPlaceCreatable, public TypeInferrable {
	friend class LocalDeclaration;

	bool             isPacked = false;
	Vec<Expression*> members;

  public:
	TupleValue(Vec<Expression*> _members, FileRangePtr _fileRange) : Expression(_fileRange), members(_members) {}

	static TupleValue* create(Vec<Expression*> _members, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(TupleValue), _members, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto mem : members) {
			UPDATE_DEPS(mem);
		}
	}

	ir::Value* emit(EmitCtx* ctx);

	NodeType nodeType() const { return NodeType::TUPLE_VALUE; }
};

} // namespace qat::ast

#endif
