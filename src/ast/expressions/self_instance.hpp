#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"
#include "../function.hpp"

namespace qat::ast {

class SelfInstance final : public Expression {
public:
  explicit SelfInstance(FileRange _fileRange) : Expression(_fileRange) {}

  useit static inline SelfInstance* create(FileRange _fileRange) {
    return std::construct_at(OwnNormal(SelfInstance), _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {}

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::SELF; }
};

} // namespace qat::ast

#endif