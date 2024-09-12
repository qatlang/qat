#include "./prerun_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

bool PrerunGenericAbstract::hasDefault() const { return defaultValueAST.has_value(); }

ir::PrerunValue* PrerunGenericAbstract::getDefault() const { return defaultValue; }

void PrerunGenericAbstract::emit(EmitCtx* ctx) const {
  SHOW("Emitting prerun generic " << name.value << " and hasDefault: " << hasDefault())
  SHOW("TypeKind for prerun param type " << (u32)expTy->type_kind());
  expressionType = expTy->emit(ctx);
  SHOW("Emitted type of prerun generic parameter")
  if (!expressionType->can_be_prerun_generic()) {
    ctx->Error("The provided type is not qualified to be used for a prerun generic expression", expTy->fileRange);
  }
  if (hasDefault()) {
    auto* astVal = defaultValueAST.value();
    if (astVal->has_type_inferrance()) {
      astVal->as_type_inferrable()->set_inference_type(expressionType);
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

ir::Type* PrerunGenericAbstract::getType() const { return expressionType; }

ir::PrerunValue* PrerunGenericAbstract::getPrerun() const {
  return !expressionValue.empty() ? expressionValue.back() : defaultValue;
}

bool PrerunGenericAbstract::isSet() const { return !expressionValue.empty() || (defaultValue != nullptr); }

void PrerunGenericAbstract::setExpression(ir::PrerunValue* exp) const { expressionValue.push_back(exp); }

void PrerunGenericAbstract::unset() const { expressionValue.pop_back(); }

ir::PrerunGeneric* PrerunGenericAbstract::toIR() const { return ir::PrerunGeneric::get(name, getPrerun(), range); }

Json PrerunGenericAbstract::to_json() const {
  return Json()
      ._("genericKind", "prerunGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultValueAST.has_value())
      ._("defaultValue", defaultValueAST.has_value() ? defaultValueAST.value()->to_json() : JsonValue())
      ._("defaultValueString", defaultValueAST.has_value() ? defaultValueAST.value()->to_string() : JsonValue())
      ._("range", range);
}

PrerunGenericAbstract::~PrerunGenericAbstract() {}

} // namespace qat::ast
