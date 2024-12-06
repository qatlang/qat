#ifndef QAT_AST_ADDRESS_OF_HPP
#define QAT_AST_ADDRESS_OF_HPP

#include "../expression.hpp"

namespace qat::ast {

class AddressOf final : public Expression {
  Expression* instance;

public:
  AddressOf(Expression* _instance, FileRange _fileRange) : Expression(_fileRange), instance(_instance) {}

  useit static AddressOf* create(Expression* _instance, FileRange _fileRange) {
    return std::construct_at(OwnNormal(AddressOf), _instance, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(instance);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit Json       to_json() const final;
  useit NodeType   nodeType() const final { return NodeType::ADDRESS_OF; }
};

} // namespace qat::ast

#endif
