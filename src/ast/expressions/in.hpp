#ifndef QAT_AST_EXPRESSIONS_IN_HPP
#define QAT_AST_EXPRESSIONS_IN_HPP

#include "../expression.hpp"

namespace qat::ast {

using InExpressionVariants = std::variant<Expression*, std::nullopt_t, Type*, Type*>;

class InExpression : public TypeInferrable, public Expression {
	Expression*          candidate;
	InExpressionVariants target;

  public:
	InExpression(Expression* _candidate, InExpressionVariants _target, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), candidate(_candidate), target(std::move(_target)) {}

	useit static InExpression* create_expression(Expression* candidate, Expression* target, FileRange fileRange) {
		return std::construct_at(OwnNormal(InExpression), candidate,
		                         InExpressionVariants(std::in_place_index<0>, target), std::move(fileRange));
	}

	useit static InExpression* create_heap(Expression* candidate, FileRange fileRange) {
		return std::construct_at(OwnNormal(InExpression), candidate,
		                         InExpressionVariants(std::in_place_index<1>, std::nullopt), std::move(fileRange));
	}

	useit static InExpression* create_region(Expression* candidate, Type* target, FileRange fileRange) {
		return std::construct_at(OwnNormal(InExpression), candidate,
		                         InExpressionVariants(std::in_place_index<2>, target), std::move(fileRange));
	}

	useit static InExpression* create_type(Expression* candidate, Type* target, FileRange fileRange) {
		return std::construct_at(OwnNormal(InExpression), candidate,
		                         InExpressionVariants(std::in_place_index<3>, target), std::move(fileRange));
	}

	TYPE_INFERRABLE_FUNCTIONS

	useit bool is_target_expression() const { return target.index() == 0u; }

	useit bool is_target_heap() const { return target.index() == 1u; }

	useit bool is_target_region() const { return target.index() == 2u; }

	useit bool is_target_type() const { return target.index() == 3u; }

	useit Expression* target_as_expression() const { return std::get<0>(target); }

	useit Type* target_as_region() const { return std::get<2>(target); }

	useit Type* target_as_type() const { return std::get<3>(target); }

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx);

	ir::Value* emit(EmitCtx* ctx);

	useit NodeType nodeType() const final { return NodeType::IN_EXPRESSION; }

	useit Json to_json() const final { return Json(); }
};

} // namespace qat::ast

#endif
