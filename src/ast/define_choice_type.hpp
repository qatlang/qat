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
  DefineChoiceType(Identifier _name, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> _fields,
                   Maybe<ast::QatType*> _providedTy, bool _areValuesUnsigned, Maybe<usize> _defaultVal,
                   Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : Node(_fileRange), name(_name), fields(_fields), areValuesUnsigned(_areValuesUnsigned), visibSpec(_visibSpec),
        defaultVal(_defaultVal), providedIntegerTy(_providedTy) {}

  useit static inline DefineChoiceType* create(Identifier                                           _name,
                                               Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> _fields,
                                               Maybe<ast::QatType*> _providedTy, bool _areValuesUnsigned,
                                               Maybe<usize> _defaultVal, Maybe<VisibilitySpec> _visibSpec,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineChoiceType), _name, _fields, _providedTy, _areValuesUnsigned, _defaultVal,
                             _visibSpec, _fileRange);
  }

  void  createType(IR::Context* ctx);
  void  defineType(IR::Context* ctx) final;
  void  define(IR::Context* ctx) final {}
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::DEFINE_CHOICE_TYPE; }
};

} // namespace qat::ast

#endif