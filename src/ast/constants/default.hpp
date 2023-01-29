#ifndef QAT_AST_CONSTANTS_DEFAULT_HPP
#define QAT_AST_CONSTANTS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ConstantDefault : public ConstantExpression {
  mutable Maybe<ast::QatType*>             theType;
  mutable Maybe<ast::GenericAbstractType*> genericAbstractType;

public:
  ConstantDefault(Maybe<ast::QatType*> _type, FileRange range);

  void setGenericAbstract(ast::GenericAbstractType* genAbs) const;

  useit IR::ConstantValue* emit(IR::Context* ctx) final;
  useit NodeType           nodeType() const final { return NodeType::constantDefault; }
  useit Json               toJson() const final;
};

} // namespace qat::ast

#endif