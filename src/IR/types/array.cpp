#include "./array.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length)
    : element_type(_element_type), length(_length) {}

QatType *ArrayType::getElementType() { return element_type; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

String ArrayType::toString() const {
  return element_type->toString() + "[" + std::to_string(length) + "]";
}

llvm::Type *ArrayType::emitLLVM(llvmHelper &helper) const {
  return llvm::ArrayType::get(element_type->emitLLVM(helper), length);
}

void ArrayType::emitCPP(cpp::File &file) const {
  element_type->emitCPP(file);
  if (!file.getArraySyntaxIsBracket()) {
    file << " *";
  } else {
    file << " ";
  }
}

} // namespace qat::IR