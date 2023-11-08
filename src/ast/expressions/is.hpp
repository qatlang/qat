#ifndef QAT_AST_EXPRESSIONS_IS_HPP
#define QAT_AST_EXPRESSIONS_IS_HPP

#include "../expression.hpp"

namespace qat::ast {

class IsExpression : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class Assignment;
  friend class LocalDeclaration;

  Expression* subExpr      = nullptr;
  bool        confirmedRef = false;
  bool        isRefVar     = false;

public:
  IsExpression(Expression* subExpr, FileRange fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::isExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif
