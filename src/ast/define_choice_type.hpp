#ifndef QAT_AST_DEFINE_CHOICE_TYPE_HPP
#define QAT_AST_DEFINE_CHOICE_TYPE_HPP

#include "./node.hpp"
#include "expression.hpp"

namespace qat::ast {

class QatType;

class DefineChoiceType : public Node {
public:
private:
  Identifier                                      name;
  Vec<Pair<Identifier, Maybe<PrerunExpression*>>> fields;
  bool                                            areValuesUnsigned;
  Maybe<VisibilitySpec>                           visibSpec;
  Maybe<usize>                                    defaultVal;
  Maybe<ast::QatType*>                            providedIntegerTy;

public:
  DefineChoiceType(Identifier name, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> fields,
                   Maybe<ast::QatType*> providedIntegerTy, bool areValuesUnsigned, Maybe<usize> defaultVal,
                   Maybe<VisibilitySpec> visibSpec, FileRange fileRange);

  void  createType(IR::Context* ctx);
  void  defineType(IR::Context* ctx) final;
  void  define(IR::Context* ctx) final {}
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::defineChoiceType; }
};

} // namespace qat::ast

#endif