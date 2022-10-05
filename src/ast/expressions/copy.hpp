#ifndef QAT_AST_EXPRESSIONS_COPY_HPP
#define QAT_AST_EXPRESSIONS_COPY_HPP

#include "../expression.hpp"

namespace qat::ast {

class Copy : public Expression {
private:
  Expression* exp;

public:
  Copy(Expression* exp, utils::FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::copyExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif