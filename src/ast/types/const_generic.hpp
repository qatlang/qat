#ifndef QAT_AST_TYPES_CONST_GENERIC_HPP
#define QAT_AST_TYPES_CONST_GENERIC_HPP

#include "../../memory_tracker.hpp"
#include "../expression.hpp"
#include "../types/qat_type.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

class ConstGeneric final : public GenericAbstractType {
  QatType*                      expTy;
  Maybe<ast::PrerunExpression*> defaultValueAST;

  mutable IR::QatType*     expressionType  = nullptr;
  mutable IR::PrerunValue* defaultValue    = nullptr;
  mutable IR::PrerunValue* expressionValue = nullptr;

  ConstGeneric(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
               FileRange _range);

public:
  static ConstGeneric* get(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                           FileRange _range);

  useit bool hasDefault() const final;
  useit IR::PrerunValue* getDefault() const;

  void emit(IR::Context* ctx) const final;

  useit IR::QatType* getType() const;
  useit IR::PrerunValue* getConstant() const;
  useit IR::PrerunGeneric* toIR() const;

  useit bool isSet() const final;
  void       setExpression(IR::PrerunValue* exp) const;
  void       unset() const final;

  ~ConstGeneric() final;
};

} // namespace qat::ast

#endif