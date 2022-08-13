#include "./value.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::IR {

Value::Value(llvm::Value *_llvmValue, IR::QatType *_type, bool _isVariable,
             Nature _kind)
    : type(_type), nature(_kind), variable(_isVariable), ll(_llvmValue) {
  allValues.push_back(this);
}

Vec<Value *> Value::allValues = {};

QatType *Value::getType() const { return type; }

llvm::Value *Value::getLLVM() { return ll; }

IR::Value *Value::createAlloca(llvm::IRBuilder<> &builder) {
  auto *alloc = builder.CreateAlloca(getType()->getLLVMType());
  builder.CreateStore(ll, alloc);
  return new IR::Value(alloc, getType(), variable, nature);
}

bool Value::isImplicitPointer() const {
  return ll && (llvm::isa<llvm::AllocaInst>(ll) ||
                llvm::isa<llvm::GlobalVariable>(ll));
}

void Value::loadImplicitPointer(llvm::IRBuilder<> &builder) {
  if (isImplicitPointer()) {
    ll = builder.CreateLoad(getType()->getLLVMType(), ll);
  }
}

bool Value::isPointer() const {
  return (type->typeKind() == IR::TypeKind::pointer);
}

bool Value::isReference() const {
  return (type->typeKind() == IR::TypeKind::reference);
}

bool Value::isVariable() const { return variable; }

Nature Value::getNature() const { return nature; }

void Value::clearAll() {
  for (auto *val : allValues) {
    delete val;
  }
  allValues.clear();
}

} // namespace qat::IR