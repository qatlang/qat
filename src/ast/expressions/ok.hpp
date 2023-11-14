#ifndef QAT_AST_EXPRESSIONS_OK_HPP
#define QAT_AST_EXPRESSIONS_OK_HPP

#include "../expression.hpp"

namespace qat::ast {

class Ok : public Expression, public LocalDeclCompatible, public InPlaceCreatable, public TypeInferrable {
  Expression* subExpr = nullptr;

public:
  Ok(Expression* subExpr, FileRange fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::okExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif