#include "./prerun_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

bool PrerunGeneric::hasDefault() const { return defaultValueAST.has_value(); }

ir::PrerunValue* PrerunGeneric::getDefault() const { return defaultValue; }

void PrerunGeneric::emit(EmitCtx* ctx) const {
  SHOW("Emitting prerun generic " << name.value << " and hasDefault: " << hasDefault())
  SHOW("TypeKind for prerun param type " << (uint)expTy->type_kind());
  expressionType = expTy->emit(ctx);
  SHOW("Emitted type of prerun generic parameter")
  if (!expressionType->can_be_prerun_generic()) {
    ctx->Error("The provided type is not qualified to be used for a prerun generic expression", expTy->fileRange);
  }
  if (hasDefault()) {
    auto* astVal = defaultValueAST.value();
    if (astVal->hasTypeInferrance()) {
      astVal->asTypeInferrable()->setInferenceType(expressionType);
    }
    defaultValue = astVal->emit(ctx);
    if (defaultValue) {
      if (!defaultValue->get_ir_type()->is_same(expressionType)) {
        ctx->Error("The expected type for the prerun generic expression is " + ctx->color(expressionType->to_string()) +
                       " but the provided expression is of type " +
                       ctx->color(defaultValue->get_ir_type()->to_string()),
                   astVal->fileRange);
      }
    } else {
      ctx->Error("No prerun expression generated from the default expression provided", astVal->fileRange);
    }
  }
}

ir::Type* PrerunGeneric::getType() const { return expressionType; }

ir::PrerunValue* PrerunGeneric::getPrerun() const {
  return !expressionValue.empty() ? expressionValue.back() : defaultValue;
}

bool PrerunGeneric::isSet() const { return !expressionValue.empty() || (defaultValue != nullptr); }

void PrerunGeneric::setExpression(ir::PrerunValue* exp) const { expressionValue.push_back(exp); }

void PrerunGeneric::unset() const { expressionValue.pop_back(); }

ir::PrerunGeneric* PrerunGeneric::toIR() const { return ir::PrerunGeneric::get(name, getPrerun(), range); }

Json PrerunGeneric::to_json() const {
  return Json()
      ._("genericKind", "prerunGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultValueAST.has_value())
      ._("defaultValue", defaultValueAST.has_value() ? defaultValueAST.value()->to_json() : Json())
      ._("range", range);
}

PrerunGeneric::~PrerunGeneric() {}

} // namespace qat::ast