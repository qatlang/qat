#ifndef QAT_AST_EXPRESSIONS_MOVE_HPP
#define QAT_AST_EXPRESSIONS_MOVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Move final : public Expression, public LocalDeclCompatible, public InPlaceCreatable {
	friend class Assignment;
	friend class LocalDeclaration;

  private:
	Expression* exp;

	bool isAssignment = false;

  public:
	Move(Expression* _exp, FileRangePtr _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

	useit static Move* create(Expression* _exp, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(Move), _exp, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(exp);
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::MOVE_EXPRESSION; }
};

} // namespace qat::ast

#endif
