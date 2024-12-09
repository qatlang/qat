#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"

#include <vector>

namespace qat::ast {

class TupleValue final : public Expression, public LocalDeclCompatible, public TypeInferrable {
	friend class LocalDeclaration;

	bool             isPacked = false;
	Vec<Expression*> members;

  public:
	TupleValue(Vec<Expression*> _members, FileRange _fileRange) : Expression(_fileRange), members(_members) {}

	useit static TupleValue* create(Vec<Expression*> _members, FileRange _fileRange) {
		return std::construct_at(OwnNormal(TupleValue), _members, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		for (auto mem : members) {
			UPDATE_DEPS(mem);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx);
	useit Json       to_json() const;
	useit NodeType   nodeType() const { return NodeType::TUPLE_VALUE; }
};

} // namespace qat::ast

#endif
