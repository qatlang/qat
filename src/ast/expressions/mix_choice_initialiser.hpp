#ifndef QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP
#define QAT_AST_EXPRESSIONS_MIX_TYPE_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class MixOrChoiceInitialiser final : public Expression,
                                     public LocalDeclCompatible,
                                     public InPlaceCreatable,
                                     public TypeInferrable {
  friend class LocalDeclaration;

private:
  Maybe<QatType*>    type;
  Identifier         subName;
  Maybe<Expression*> expression;

public:
  MixOrChoiceInitialiser(Maybe<QatType*> _type, Identifier _subName, Maybe<Expression*> _expression,
                         FileRange _fileRange)
      : Expression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

  useit static inline MixOrChoiceInitialiser* create(Maybe<QatType*> type, Identifier subName,
                                                     Maybe<Expression*> expression, FileRange fileRange) {
    return std::construct_at(OwnNormal(MixOrChoiceInitialiser), type, subName, expression, fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    if (type.has_value()) {
      UPDATE_DEPS(type.value());
    }
    if (expression.has_value()) {
      UPDATE_DEPS(expression.value());
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::MIX_OR_CHOICE_INITIALISER; }
};

} // namespace qat::ast

#endif