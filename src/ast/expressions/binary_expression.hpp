#ifndef QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP
#define QAT_AST_EXPRESSIONS_BINARY_EXPRESSION_HPP

#include "../expression.hpp"
#include "./operator.hpp"
#include <utility>

namespace qat::ast {

class BinaryExpression final : public Expression {
private:
  Op          op;
  Expression* lhs;
  Expression* rhs;

public:
  BinaryExpression(Expression* _lhs, const String& _binaryOperator, Expression* _rhs, FileRange _fileRange)
      : Expression(std::move(_fileRange)), op(operator_from_string(_binaryOperator)), lhs(_lhs), rhs(_rhs) {}

  useit static inline BinaryExpression* create(Expression* _lhs, const String& _binaryOperator, Expression* _rhs,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(BinaryExpression), _lhs, _binaryOperator, _rhs, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(lhs);
    UPDATE_DEPS(rhs);
  }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::BINARY_EXPRESSION; }
};

} // namespace qat::ast

#endif