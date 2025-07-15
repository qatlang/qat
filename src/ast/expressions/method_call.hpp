#ifndef QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP

#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class MethodCall final : public Expression {
  private:
	Expression*                     instance;
	bool                            isExpSelf = false;
	Identifier                      memberName;
	Vec<Expression*>                arguments;
	Maybe<Pair<bool, FileRangePtr>> callNature;

  public:
	MethodCall(Expression* _instance, bool _isExpSelf, Identifier _memberName, Vec<Expression*> _arguments,
	           Maybe<Pair<bool, FileRangePtr>> _callNature, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), instance(_instance), isExpSelf(_isExpSelf),
	      memberName(std::move(_memberName)), arguments(std::move(_arguments)), callNature(_callNature) {}

	useit static MethodCall* create(Expression* _instance, bool _isExpSelf, Identifier _memberName,
	                                Vec<Expression*> _arguments, Maybe<Pair<bool, FileRangePtr>> _callNature,
	                                FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MethodCall), _instance, _isExpSelf, _memberName, _arguments, _callNature,
		                         _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) override;
	useit Json       to_json() const override;

	useit NodeType nodeType() const override { return NodeType::MEMBER_FUNCTION_CALL; }
};

} // namespace qat::ast

#endif
