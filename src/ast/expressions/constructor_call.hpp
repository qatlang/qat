#ifndef QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP
#define QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ConstructorCall final : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;

private:
  Maybe<QatType*>  type;
  Vec<Expression*> args;

public:
  ConstructorCall(Maybe<QatType*> _type, Vec<Expression*> _args, FileRange _fileRange)
      : Expression(_fileRange), type(_type), args(_args) {}

  useit static inline ConstructorCall* create(Maybe<QatType*> _type, Vec<Expression*> _args, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ConstructorCall), _type, _args, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    if (type.has_value()) {
      type.value()->update_dependencies(phase, IR::DependType::childrenPartial, ent, ctx);
    }
    for (auto arg : args) {
      UPDATE_DEPS(arg);
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::CONSTRUCTOR_CALL; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif