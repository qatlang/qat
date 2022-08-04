#include "./value.hpp"
#include "./types/qat_type.hpp"

namespace qat::IR {

Value::Value(IR::QatType *_type, bool _isVariable, Nature _kind)
    : type(_type), variable(_isVariable), nature(_kind) {}

QatType *Value::getType() const { return type; }

bool Value::isPointer() const {
  return (type->typeKind() == IR::TypeKind::pointer);
}

bool Value::isReference() const {
  return (type->typeKind() == IR::TypeKind::reference);
}

bool Value::isVariable() const { return variable; }

Nature Value::getKind() const { return nature; }

} // namespace qat::IR