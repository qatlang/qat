#include "./prerun_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

PrerunGeneric::PrerunGeneric(usize _index, Identifier _name, QatType* _expTy, Maybe<ast::PrerunExpression*> _defaultVal,
                             FileRange _range)
    : GenericAbstractType(_index, std::move(_name), GenericKind::prerunGeneric, std::move(_range)), expTy(_expTy),
      defaultValueAST(_defaultVal) {}

PrerunGeneric* PrerunGeneric::get(usize _index, Identifier _name, QatType* _expTy,
                                  Maybe<ast::PrerunExpression*> _defaultVal, FileRange _range) {
  return new PrerunGeneric(_index, std::move(_name), _expTy, _defaultVal, std::move(_range));
}

bool PrerunGeneric::hasDefault() const { return defaultValueAST.has_value(); }

IR::PrerunValue* PrerunGeneric::getDefault() const { return defaultValue; }

void PrerunGeneric::emit(IR::Context* ctx) const {
  expressionType = expTy->emit(ctx);
  if (!expressionType->canBePrerunGeneric()) {
    ctx->Error("The provided type is not qualified to be used for a prerun generic expression", expTy->fileRange);
  }
  if (hasDefault()) {
    auto* astVal = defaultValueAST.value();
    if (astVal->hasTypeInferrance()) {
      astVal->asTypeInferrable()->setInferenceType(expressionType);
    }
    defaultValue = astVal->emit(ctx);
    if (defaultValue) {
      if (!defaultValue->getType()->isSame(expressionType)) {
        ctx->Error("The expected type for the prerun generic expression is " +
                       ctx->highlightError(expressionType->toString()) + " but the provided expression is of type " +
                       ctx->highlightError(defaultValue->getType()->toString()),
                   astVal->fileRange);
      }
    } else {
      ctx->Error("No prerun expression generated from the default expression provided", astVal->fileRange);
    }
  }
}

IR::QatType* PrerunGeneric::getType() const { return expressionType; }

IR::PrerunValue* PrerunGeneric::getPrerun() const { return expressionValue ? expressionValue : defaultValue; }

bool PrerunGeneric::isSet() const { return (expressionValue != nullptr) || (defaultValue != nullptr); }

void PrerunGeneric::setExpression(IR::PrerunValue* exp) const { expressionValue = exp; }

void PrerunGeneric::unset() const { expressionValue = nullptr; }

IR::PrerunGeneric* PrerunGeneric::toIR() const { return IR::PrerunGeneric::get(name, getPrerun(), range); }

Json PrerunGeneric::toJson() const {
  return Json()
      ._("genericKind", "prerunGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultValueAST.has_value())
      ._("defaultValue", defaultValueAST.has_value() ? defaultValueAST.value()->toJson() : Json())
      ._("range", range);
}

PrerunGeneric::~PrerunGeneric() {}

} // namespace qat::ast