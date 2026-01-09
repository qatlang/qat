#ifndef QAT_AST_EXPRESSIONS_CONFIRM_REF_HPP
#define QAT_AST_EXPRESSIONS_CONFIRM_REF_HPP

#include "../expression.hpp"

namespace qat::ast {

class ConfirmRef : public Expression {
	Expression* subExpr;
	bool        isVar;

  public:
	ConfirmRef(Expression* _subExpr, bool _isVar, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), subExpr(_subExpr), isVar(_isVar) {}

	static ConfirmRef* create(Expression* subExpr, bool isVar, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(ConfirmRef), subExpr, isVar, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	ir::Value* emit(EmitCtx* emitCtx);

	NodeType nodeType() const final { return NodeType::CONFIRM_REF; }
};

} // namespace qat::ast

#endif
