#ifndef QAT_AST_EXPRESSIONS_DEREFERENCE_HPP
#define QAT_AST_EXPRESSIONS_DEREFERENCE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Dereference : public Expression {
private:
  Expression* exp;

public:
  Dereference(Expression* _exp, FileRange _fileRange) : Expression(_fileRange), exp(_exp) {}

  useit static inline Dereference* create(Expression* _exp, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Dereference), _exp, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::DEREFERENCE; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif