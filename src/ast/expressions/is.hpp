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
  IsExpression(Expression* _subExpr, FileRange _fileRange) : Expression(_fileRange), subExpr(_subExpr) {}

  useit static inline IsExpression* create(Expression* subExpr, FileRange fileRange) {
    return std::construct_at(OwnNormal(IsExpression), subExpr, fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::IS; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif
