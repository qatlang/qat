#ifndef QAT_AST_CONSTANTS_DEFAULT_HPP
#define QAT_AST_CONSTANTS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunDefault : public PrerunExpression, public TypeInferrable {
  mutable Maybe<ast::QatType*>             theType;
  mutable Maybe<ast::GenericAbstractType*> genericAbstractType;

public:
  PrerunDefault(Maybe<ast::QatType*> _type, FileRange range);

  void setGenericAbstract(ast::GenericAbstractType* genAbs) const;

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit NodeType         nodeType() const final { return NodeType::prerunDefault; }
  useit Json             toJson() const final;
};

} // namespace qat::ast

#endif