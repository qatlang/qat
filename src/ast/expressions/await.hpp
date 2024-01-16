#ifndef QAT_AST_EXPRESSIONS_AWAIT_HPP
#define QAT_AST_EXPRESSIONS_AWAIT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Await : public Expression {
private:
  Expression* exp;

public:
  Await(Expression* _exp, FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

  useit static inline Await* create(Expression* exp, FileRange fileRange) {
    return std::construct_at(OwnNormal(Await), exp, fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::AWAIT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif