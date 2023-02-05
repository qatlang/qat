#include "./generic_abstract.hpp"
#include "./const_generic.hpp"
#include "./typed_generic.hpp"
#include "qat_type.hpp"

namespace qat::ast {

GenericAbstractType::GenericAbstractType(usize _index, Identifier _name, GenericKind _kind, FileRange _range)
    : index(_index), name(std::move(_name)), kind(_kind), range(std::move(_range)) {
  ast::QatType::generics.push_back(this);
}

usize GenericAbstractType::getIndex() const { return index; }

Identifier GenericAbstractType::getName() const { return name; }

FileRange GenericAbstractType::getRange() const { return range; }

bool GenericAbstractType::isConst() const { return kind == GenericKind::constGeneric; }

TypedGeneric* GenericAbstractType::asTyped() const { return (TypedGeneric*)this; }

bool GenericAbstractType::isTyped() const { return kind == GenericKind::typedGeneric; }

ConstGeneric* GenericAbstractType::asConst() const { return (ConstGeneric*)this; }

IR::GenericType* GenericAbstractType::toIRGenericType() const {
  if (isTyped()) {
    return asTyped()->toIR();
  } else {
    return asConst()->toIR();
  }
}

} // namespace qat::ast