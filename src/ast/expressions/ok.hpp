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

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(subExpr);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::OK; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif