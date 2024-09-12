#include "./typed_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

bool TypedGenericAbstract::hasDefault() const { return defaultTypeAST.has_value(); }

ir::Type* TypedGenericAbstract::getDefault() const { return defaultType; }

void TypedGenericAbstract::setType(ir::Type* typ) const { typeValue.push_back(typ); }

bool TypedGenericAbstract::isSet() const { return !typeValue.empty() || (defaultType != nullptr); }

void TypedGenericAbstract::unset() const { typeValue.pop_back(); }

void TypedGenericAbstract::emit(EmitCtx* ctx) const {
  SHOW("Emitting typed generic " << name.value << " and hasDefault: " << hasDefault())
  if (hasDefault()) {
    defaultType = defaultTypeAST.value()->emit(ctx);
  }
}

ir::TypedGeneric* TypedGenericAbstract::toIR() const { return ir::TypedGeneric::get(name, get_type(), range); }

ir::Type* TypedGenericAbstract::get_type() const { return !typeValue.empty() ? typeValue.back() : defaultType; }

Json TypedGenericAbstract::to_json() const {
  return Json()
      ._("genericKind", "typedGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultTypeAST.has_value())
      ._("defaultType", defaultTypeAST.has_value() ? defaultTypeAST.value()->to_json() : JsonValue())
      ._("defaultValueString", defaultTypeAST.has_value() ? defaultTypeAST.value()->to_string() : JsonValue())
      ._("range", range);
}

TypedGenericAbstract::~TypedGenericAbstract() {}

} // namespace qat::ast
