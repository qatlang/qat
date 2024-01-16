#ifndef QAT_AST_CONSTANTS_DEFAULT_HPP
#define QAT_AST_CONSTANTS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunDefault : public PrerunExpression, public TypeInferrable {
  mutable Maybe<ast::QatType*>             theType;
  mutable Maybe<ast::GenericAbstractType*> genericAbstractType;

public:
  PrerunDefault(Maybe<ast::QatType*> _type, FileRange range) : PrerunExpression(range), theType(_type) {}

  useit static inline PrerunDefault* create(Maybe<ast::QatType*> _type, FileRange _range) {
    return std::construct_at(OwnNormal(PrerunDefault), _type, _range);
  }

  void setGenericAbstract(ast::GenericAbstractType* genAbs) const;

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_DEFAULT; }
  useit Json             toJson() const final;
};

} // namespace qat::ast

#endif