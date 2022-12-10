#ifndef QAT_AST_EXPRESSIONS_NOT_HPP
#define QAT_AST_EXPRESSIONS_NOT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Not : public Expression {
  Expression* exp;

public:
  Not(Expression* exp, FileRange range);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Not; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif