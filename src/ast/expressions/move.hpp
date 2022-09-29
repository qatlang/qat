#ifndef QAT_AST_EXPRESSIONS_MOVE_HPP
#define QAT_AST_EXPRESSIONS_MOVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Move : public Expression {
  Expression *exp;

public:
  Move(Expression *exp, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::moveExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif