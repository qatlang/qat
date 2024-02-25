#ifndef QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_ARRAY_LITERAL_HPP

#include "../expression.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

class ArrayLiteral final : public Expression,
                           public LocalDeclCompatible,
                           public InPlaceCreatable,
                           public TypeInferrable {
  friend class LocalDeclaration;
  friend class Assignment;

private:
  Vec<Expression*> values;

public:
  ArrayLiteral(Vec<Expression*> _values, FileRange _fileRange)
      : Expression(std::move(_fileRange)), values(std::move(_values)) {}

  useit static inline ArrayLiteral* create(Vec<Expression*> values, FileRange fileRange) {
    return std::construct_at(OwnNormal(ArrayLiteral), values, fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    for (auto it : values) {
      UPDATE_DEPS(it);
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::ARRAY_LITERAL; }
};

} // namespace qat::ast

#endif