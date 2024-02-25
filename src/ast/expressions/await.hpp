#ifndef QAT_AST_EXPRESSIONS_AWAIT_HPP
#define QAT_AST_EXPRESSIONS_AWAIT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Await final : public Expression {
private:
  Expression* exp;

public:
  Await(Expression* _exp, FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

  useit static inline Await* create(Expression* exp, FileRange fileRange) {
    return std::construct_at(OwnNormal(Await), exp, fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::AWAIT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif