#ifndef QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP
#define QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP

#include "../expression.hpp"
#include <string>
#include <utility>

namespace qat::ast {

class BinaryExpression : public Expression {
private:
  String      op;
  Expression *lhs;
  Expression *rhs;

public:
  BinaryExpression(Expression *_lhs, String _binaryOperator, Expression *_rhs,
                   utils::FileRange _fileRange)
      : lhs(_lhs), op(std::move(_binaryOperator)), rhs(_rhs),
        Expression(std::move(_fileRange)) {}

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::binaryExpression;
  }
};

} // namespace qat::ast

#endif