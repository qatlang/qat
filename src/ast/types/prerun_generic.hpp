#ifndef QAT_AST_TYPES_PRERUN_GENERIC_HPP
#define QAT_AST_TYPES_PRERUN_GENERIC_HPP

#include "../../memory_tracker.hpp"
#include "../expression.hpp"
#include "../types/qat_type.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

class PrerunGeneric final : public GenericAbstractType {
  QatType*                      expTy;
  Maybe<ast::PrerunExpression*> defaultValueAST;

  mutable IR::QatType*     expressionType  = nullptr;
  mutable IR::PrerunValue* defaultValue    = nullptr;
  mutable IR::PrerunValue* expressionValue = nullptr;

  PrerunGeneric(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                FileRange _range);

public:
  static PrerunGeneric* get(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                            FileRange _range);

  useit bool hasDefault() const final;
  useit IR::PrerunValue* getDefault() const;

  void emit(IR::Context* ctx) const final;

  useit IR::QatType* getType() const;
  useit IR::PrerunValue* getPrerun() const;
  useit IR::PrerunGeneric* toIR() const;

  useit bool isSet() const final;
  void       setExpression(IR::PrerunValue* exp) const;
  void       unset() const final;
  useit Json toJson() const final;

  ~PrerunGeneric() final;
};

} // namespace qat::ast

#endif