#ifndef QAT_AST_EXPRESSIONS_DEREFERENCE_HPP
#define QAT_AST_EXPRESSIONS_DEREFERENCE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Dereference : public Expression {
private:
  Expression* exp;

public:
  Dereference(Expression* exp, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::dereference; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif