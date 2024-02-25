#ifndef QAT_AST_DEFINE_CHOICE_TYPE_HPP
#define QAT_AST_DEFINE_CHOICE_TYPE_HPP

#include "./node.hpp"
#include "expression.hpp"

namespace qat::ast {

class QatType;

class DefineChoiceType : public IsEntity {
private:
  Identifier                                      name;
  Vec<Pair<Identifier, Maybe<PrerunExpression*>>> fields;
  Maybe<VisibilitySpec>                           visibSpec;
  Maybe<usize>                                    defaultVal;
  Maybe<ast::QatType*>                            providedIntegerTy;

  mutable bool             areValuesUnsigned;
  mutable IR::EntityState* entityState = nullptr;

public:
  DefineChoiceType(Identifier _name, Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> _fields,
                   Maybe<ast::QatType*> _providedTy, Maybe<usize> _defaultVal, Maybe<VisibilitySpec> _visibSpec,
                   FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), fields(_fields), visibSpec(_visibSpec), defaultVal(_defaultVal),
        providedIntegerTy(_providedTy) {}

  useit static inline DefineChoiceType* create(Identifier                                           _name,
                                               Vec<Pair<Identifier, Maybe<ast::PrerunExpression*>>> _fields,
                                               Maybe<ast::QatType*> _providedTy, Maybe<usize> _defaultVal,
                                               Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineChoiceType), _name, _fields, _providedTy, _defaultVal, _visibSpec,
                             _fileRange);
  }

  void create_entity(IR::QatModule* mod, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) final;

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::DEFINE_CHOICE_TYPE; }
};

} // namespace qat::ast

#endif