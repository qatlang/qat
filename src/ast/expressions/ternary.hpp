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
  Expression* condition;
  Expression* trueExpr;
  Expression* falseExpr;

public:
  TernaryExpression(Expression* _condition, Expression* _trueExpr, Expression* _falseExpr, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::ternaryExpression; }
};

} // namespace qat::ast

#endif