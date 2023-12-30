#ifndef QAT_AST_EXPRESSIONS_MOVE_HPP
#define QAT_AST_EXPRESSIONS_MOVE_HPP

#include "../expression.hpp"

namespace qat::ast {

class Move : public Expression, public LocalDeclCompatible, public InPlaceCreatable {
  friend class Assignment;
  friend class LocalDeclaration;

private:
  Expression* exp;
  bool        isExpSelf = false;

  bool isAssignment = false;

public:
  Move(Expression* exp, bool _isExpSelf, FileRange fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::moveExpression; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif