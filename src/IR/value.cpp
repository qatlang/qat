#include "./value.hpp"
#include "./types/qat_type.hpp"

namespace qat::IR {

Value::Value(llvm::Value *_llvmValue, IR::QatType *_type, bool _isVariable,
             Nature _kind)
    : ll(_llvmValue), type(_type), variable(_isVariable), nature(_kind) {}

QatType *Value::getType() const { return type; }

llvm::Value *Value::getLLVM() { return ll; }

bool Value::isPointer() const {
  return (type->typeKind() == IR::TypeKind::pointer);
}

bool Value::isReference() const {
  return (type->typeKind() == IR::TypeKind::reference);
}

bool Value::isVariable() const { return variable; }

Nature Value::getNature() const { return nature; }

} // namespace qat::IR