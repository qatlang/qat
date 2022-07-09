#include "./value.hpp"
#include "./types/qat_type.hpp"

namespace qat {
namespace IR {

Value::Value(IR::QatType *_type, bool _isVariable, Kind _kind)
    : type(_type), variable(_isVariable), kind(_kind) {}

const IR::QatType *Value::getType() const { return type; }

bool Value::isPointer() const {
  return (type->typeKind() == IR::TypeKind::pointer);
}

bool Value::isReference() const {
  return (type->typeKind() == IR::TypeKind::reference);
}

bool Value::isVariable() const { return variable; }

Kind Value::getKind() const { return kind; }

} // namespace IR
} // namespace qat