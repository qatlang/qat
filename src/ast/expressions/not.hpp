#ifndef QAT_AST_EXPRESSIONS_NOT_HPP
#define QAT_AST_EXPRESSIONS_NOT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Not : public Expression {
  Expression* exp;

public:
  Not(Expression* _exp, FileRange _range) : Expression(_range), exp(_exp) {}

  useit static inline Not* create(Expression* _exp, FileRange _range) {
    return std::construct_at(OwnNormal(Not), _exp, _range);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::NOT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif