#ifndef QAT_AST_EXPRESSIONS_AWAIT_HPP
#define QAT_AST_EXPRESSIONS_AWAIT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Await : public Expression {
private:
  Expression* exp;

public:
  Await(Expression* exp, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Await; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif