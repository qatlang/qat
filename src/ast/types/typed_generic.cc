#include "./typed_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

bool TypedGeneric::hasDefault() const { return defaultTypeAST.has_value(); }

ir::Type* TypedGeneric::getDefault() const { return defaultType; }

void TypedGeneric::setType(ir::Type* typ) const { typeValue.push_back(typ); }

bool TypedGeneric::isSet() const { return !typeValue.empty() || (defaultType != nullptr); }

void TypedGeneric::unset() const { typeValue.pop_back(); }

void TypedGeneric::emit(EmitCtx* ctx) const {
  SHOW("Emitting typed generic " << name.value << " and hasDefault: " << hasDefault())
  if (hasDefault()) {
    defaultType = defaultTypeAST.value()->emit(ctx);
  }
}

ir::TypedGeneric* TypedGeneric::toIR() const { return ir::TypedGeneric::get(name, get_type(), range); }

ir::Type* TypedGeneric::get_type() const { return !typeValue.empty() ? typeValue.back() : defaultType; }

Json TypedGeneric::to_json() const {
  return Json()
      ._("genericKind", "typedGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultTypeAST.has_value())
      ._("defaultType", defaultTypeAST.has_value() ? defaultTypeAST.value()->to_json() : Json())
      ._("range", range);
}

TypedGeneric::~TypedGeneric() {}

} // namespace qat::ast