#ifndef QAT_AST_TYPES_PRERUN_GENERIC_HPP
#define QAT_AST_TYPES_PRERUN_GENERIC_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

class PrerunGeneric final : public GenericAbstractType {
  Type*                         expTy;
  Maybe<ast::PrerunExpression*> defaultValueAST;

  mutable ir::Type*             expressionType = nullptr;
  mutable ir::PrerunValue*      defaultValue   = nullptr;
  mutable Vec<ir::PrerunValue*> expressionValue;

public:
  PrerunGeneric(usize _index, Identifier _name, Type* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                FileRange _range)
      : GenericAbstractType(_index, _name, GenericKind::prerunGeneric, _range), expTy(_expTy),
        defaultValueAST(_defaultVal) {}

  useit static inline PrerunGeneric* get(usize _index, Identifier _name, Type* _expTy,
                                         Maybe<ast::PrerunExpression*> _defaultVal, FileRange _range) {
    return std::construct_at(OwnNormal(PrerunGeneric), _index, std::move(_name), _expTy, _defaultVal,
                             std::move(_range));
  }

  useit bool hasDefault() const final;
  useit ast::PrerunExpression* getDefaultAST() { return defaultValueAST.value(); }
  useit ir::PrerunValue* getDefault() const;

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                           EmitCtx* ctx) final {
    UPDATE_DEPS(expTy);
    if (defaultValueAST.has_value()) {
      UPDATE_DEPS(defaultValueAST.value());
    }
  }

  void emit(EmitCtx* ctx) const final;

  useit ir::Type* getType() const;
  useit ir::PrerunValue* getPrerun() const;
  useit ir::PrerunGeneric* toIR() const;

  useit bool isSet() const final;
  void       setExpression(ir::PrerunValue* exp) const;
  void       unset() const final;
  useit Json to_json() const final;

  ~PrerunGeneric() final;
};

} // namespace qat::ast

#endif