#ifndef QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class MixTypeInitialiser : public Expression, public LocalDeclCompatible, public InPlaceCreatable {
  friend class LocalDeclaration;

private:
  QatType*           type;
  String             subName;
  Maybe<Expression*> expression;

public:
  MixTypeInitialiser(QatType* type, String subName, Maybe<Expression*> expression, FileRange fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::mixTypeInitialiser; }
};

} // namespace qat::ast

#endif