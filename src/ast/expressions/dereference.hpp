#ifndef QAT_AST_EXPRESSIONS_DEREFERENCE_HPP
#define QAT_AST_EXPRESSIONS_DEREFERENCE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Dereference final : public Expression {
private:
  Expression* exp;

public:
  Dereference(Expression* _exp, FileRange _fileRange) : Expression(_fileRange), exp(_exp) {}

  useit static inline Dereference* create(Expression* _exp, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Dereference), _exp, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::DEREFERENCE; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif