#include "./generic_abstract.hpp"
#include "./prerun_generic.hpp"
#include "./typed_generic.hpp"
#include "qat_type.hpp"

namespace qat::ast {

usize GenericAbstractType::getIndex() const { return index; }

Identifier GenericAbstractType::get_name() const { return name; }

FileRange GenericAbstractType::get_range() const { return range; }

bool GenericAbstractType::is_prerun() const { return kind == GenericKind::prerunGeneric; }

TypedGeneric* GenericAbstractType::as_typed() const { return (TypedGeneric*)this; }

bool GenericAbstractType::is_typed() const { return kind == GenericKind::typedGeneric; }

PrerunGeneric* GenericAbstractType::as_prerun() const { return (PrerunGeneric*)this; }

ir::GenericParameter* GenericAbstractType::toIRGenericType() const {
  if (is_typed()) {
    return as_typed()->toIR();
  } else {
    return as_prerun()->toIR();
  }
}

} // namespace qat::ast