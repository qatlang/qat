#ifndef QAT_AST_EXPRESSIONS_COPY_HPP
#define QAT_AST_EXPRESSIONS_COPY_HPP

#include "../expression.hpp"

namespace qat::ast {

class Copy : public Expression {
  friend class Assignment;
  friend class LocalDeclaration;

private:
  Expression*       exp;
  IR::LocalValue*   local = nullptr;
  Maybe<Identifier> irName;
  bool              isVar = false;

public:
  Copy(Expression* exp, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::copyExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif