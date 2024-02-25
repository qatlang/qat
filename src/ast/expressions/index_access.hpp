#ifndef QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_INDEX_ACCESS_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class IndexAccess final : public Expression {
  Expression* instance;
  Expression* index;

public:
  IndexAccess(Expression* _instance, Expression* _index, FileRange _fileRange)
      : Expression(_fileRange), instance(_instance), index(_index) {}

  useit static inline IndexAccess* create(Expression* _instance, Expression* _index, FileRange _fileRange) {
    return std::construct_at(OwnNormal(IndexAccess), _instance, _index, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(instance);
    UPDATE_DEPS(index);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::INDEX_ACCESS; }
};

} // namespace qat::ast

#endif