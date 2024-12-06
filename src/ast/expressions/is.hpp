#ifndef QAT_AST_EXPRESSIONS_IS_HPP
#define QAT_AST_EXPRESSIONS_IS_HPP

#include "../expression.hpp"

namespace qat::ast {

class IsExpression final : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class Assignment;
  friend class LocalDeclaration;

  Expression* subExpr      = nullptr;
  bool        confirmedRef = false;
  bool        isRefVar     = false;

public:
  IsExpression(Expression* _subExpr, FileRange _fileRange) : Expression(_fileRange), subExpr(_subExpr) {}

  useit static IsExpression* create(Expression* subExpr, FileRange fileRange) {
    return std::construct_at(OwnNormal(IsExpression), subExpr, fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(subExpr);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::IS; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
