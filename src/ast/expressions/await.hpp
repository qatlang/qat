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

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::AWAIT; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif