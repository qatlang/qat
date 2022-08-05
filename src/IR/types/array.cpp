#include "./array.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArrayType::ArrayType(QatType *_element_type, u64 _length,
                     llvm::LLVMContext &ctx)
    : element_type(_element_type), length(_length) {
  llvmType = llvm::ArrayType::get(element_type->getLLVMType(), length);
}

ArrayType *ArrayType::get(QatType *elementType, u64 _length,
                          llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->isArray()) {
      if (typ->asArray()->getLength() == _length) {
        if (typ->asArray()->getElementType()->isSame(elementType)) {
          return typ->asArray();
        }
      }
    }
  }
  return new ArrayType(elementType, _length, ctx);
}

QatType *ArrayType::getElementType() { return element_type; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

String ArrayType::toString() const {
  return element_type->toString() + "[" + std::to_string(length) + "]";
}

nuo::Json ArrayType::toJson() const {
  return nuo::Json()
      ._("type", "array")
      ._("subtype", element_type->getID())
      ._("length", std::to_string(length));
}

} // namespace qat::IR