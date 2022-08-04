#ifndef QAT_AST_EXPRESSIONS_TERNARY_HPP
#define QAT_AST_EXPRESSIONS_TERNARY_HPP

#include "../expression.hpp"
#include <vector>

namespace qat::ast {

/**
 * TernaryExpression represents an expression created by the ternary
 * operator. This requires 3 expressions. The first one is the condition on
 * which the operation takes place. If the condition is true, the second
 * expression is the result. If the condition is false, the third expression is
 * the result
 *
 */
class TernaryExpression : public Expression {
private:
  // The condition that determines the result of this expression
  Expression *condition;

  // The expression to be used if the provided condition is true
  Expression *if_expr;

  // The expression to be used if the provided condition is false
  Expression *else_expr;

public:
  // Construct a new Ternary Expression object.
  // This represents a ternary expression in the language
  TernaryExpression(Expression *_condition, Expression *_ifExpression,
                    Expression *_elseExpression, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::ternaryExpression;
  }
};

} // namespace qat::ast

#endif