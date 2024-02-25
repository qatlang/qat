#ifndef QAT_AST_EXPRESSIONS_OK_HPP
#define QAT_AST_EXPRESSIONS_OK_HPP

#include "../expression.hpp"

namespace qat::ast {

class OkExpression final : public Expression,
                           public LocalDeclCompatible,
                           public InPlaceCreatable,
                           public TypeInferrable {
  Expression* subExpr = nullptr;

public:
  OkExpression(Expression* _subExpr, FileRange _fileRange) : Expression(_fileRange), subExpr(_subExpr){};

  useit static inline OkExpression* create(Expression* _subExpr, FileRange _fileRange) {
    return std::construct_at(OwnNormal(OkExpression), _subExpr, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(subExpr);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::OK; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif