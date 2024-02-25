#ifndef QAT_AST_EXPRESSIONS_LOGICAL_NOT_HPP
#define QAT_AST_EXPRESSIONS_LOGICAL_NOT_HPP

#include "../expression.hpp"

namespace qat::ast {

class LogicalNot final : public Expression {
  Expression* exp;

public:
  LogicalNot(Expression* _exp, FileRange _range) : Expression(_range), exp(_exp) {}

  useit static inline LogicalNot* create(Expression* _exp, FileRange _range) {
    return std::construct_at(OwnNormal(LogicalNot), _exp, _range);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::NOT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif