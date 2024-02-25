#ifndef QAT_AST_CAST_HPP
#define QAT_AST_CAST_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Cast final : public Expression {
  Expression* instance;
  QatType*    destination;

public:
  Cast(Expression* _mainExp, QatType* _dest, FileRange _fileRange)
      : Expression(_fileRange), instance(_mainExp), destination(_dest) {}

  useit static inline Cast* create(Expression* mainExp, QatType* value, FileRange fileRange) {
    return std::construct_at(OwnNormal(Cast), mainExp, value, fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(destination);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::CAST; }
};

} // namespace qat::ast

#endif