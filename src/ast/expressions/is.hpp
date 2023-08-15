#ifndef QAT_AST_EXPRESSIONS_IS_HPP
#define QAT_AST_EXPRESSIONS_IS_HPP

#include "../expression.hpp"

namespace qat::ast {

class IsExpression : public Expression {
  friend class Assignment;
  friend class LocalDeclaration;

  Expression* subExpr      = nullptr;
  bool        confirmedRef = false;
  bool        isRefVar     = false;

  IR::LocalValue*   local = nullptr;
  Maybe<Identifier> irName;
  bool              isVar = false;

public:
  IsExpression(Expression* subExpr, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::isExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif
