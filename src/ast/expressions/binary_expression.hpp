#ifndef QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP
#define QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP

#include "../expression.hpp"
#include "./operator.hpp"
#include <string>
#include <utility>

namespace qat::ast {

class BinaryExpression : public Expression {
private:
  Op          op;
  Expression *lhs;
  Expression *rhs;

public:
  BinaryExpression(Expression *_lhs, const String &_binaryOperator,
                   Expression *_rhs, utils::FileRange _fileRange)
      : Expression(std::move(_fileRange)), op(OpFromString(_binaryOperator)),
        lhs(_lhs), rhs(_rhs) {}

  useit IR::Value *emit(IR::Context *ctx) override;

  useit Json toJson() const override;

  useit NodeType nodeType() const override {
    return NodeType::binaryExpression;
  }
};

} // namespace qat::ast

#endif