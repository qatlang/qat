#include "./const_generic.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

ConstGeneric::ConstGeneric(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                           FileRange _range)
    : GenericAbstractType(_index, std::move(_name), GenericKind::constGeneric, std::move(_range)), expTy(_expTy),
      defaultValueAST(_defaultVal) {}

ConstGeneric* ConstGeneric::get(usize _index, Identifier _name, QatType* _expTy,
                                Maybe<ast::PrerunExpression*> _defaultVal, FileRange _range) {
  return new ConstGeneric(_index, std::move(_name), _expTy, _defaultVal, std::move(_range));
}

bool ConstGeneric::hasDefault() const { return defaultValueAST.has_value(); }

IR::PrerunValue* ConstGeneric::getDefault() const { return defaultValue; }

void ConstGeneric::emit(IR::Context* ctx) const {
  expressionType = expTy->emit(ctx);
  if (!expressionType->canBePrerunGeneric()) {
    ctx->Error("The provided type is not qualified to be used for a const generic expression", expTy->fileRange);
  }
  if (hasDefault()) {
    auto* astVal = defaultValueAST.value();
    astVal->setInferenceType(expressionType);
    defaultValue = astVal->emit(ctx);
    if (defaultValue) {
      if (!defaultValue->getType()->isSame(expressionType)) {
        ctx->Error("The expected type for the const generic expression is " +
                       ctx->highlightError(expressionType->toString()) + " but the provided expression is of type " +
                       ctx->highlightError(defaultValue->getType()->toString()),
                   astVal->fileRange);
      }
    } else {
      ctx->Error("No const expression generated from the default expression provided", astVal->fileRange);
    }
  }
}

IR::QatType* ConstGeneric::getType() const { return expressionType; }

IR::PrerunValue* ConstGeneric::getConstant() const { return expressionValue ? expressionValue : defaultValue; }

bool ConstGeneric::isSet() const { return (expressionValue != nullptr) || (defaultValue != nullptr); }

void ConstGeneric::setExpression(IR::PrerunValue* exp) const { expressionValue = exp; }

void ConstGeneric::unset() const { expressionValue = nullptr; }

IR::PrerunGeneric* ConstGeneric::toIR() const { return IR::PrerunGeneric::get(name, getConstant(), range); }

ConstGeneric::~ConstGeneric() {}

} // namespace qat::ast