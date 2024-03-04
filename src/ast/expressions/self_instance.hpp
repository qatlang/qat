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

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {}

  useit ir::Value* emit(EmitCtx* ctx) override;
  useit Json       to_json() const override;
  useit NodeType   nodeType() const override { return NodeType::SELF; }
};

} // namespace qat::ast

#endif