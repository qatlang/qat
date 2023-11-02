#ifndef QAT_AST_EXPRESSIONS_MOVE_HPP
#define QAT_AST_EXPRESSIONS_MOVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Move : public Expression {
  friend class Assignment;
  friend class LocalDeclaration;

private:
  Expression* exp;

  bool isAssignment = false;

  Maybe<Identifier> irName;
  bool              isVar = false;

public:
  Move(Expression* exp, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::moveExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif