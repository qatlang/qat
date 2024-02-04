#include "./vector.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

VectorType::VectorType(IR::QatType* _subType, usize _count, VectorKind _kind, IR::Context* ctx)
    : subType(_subType), count(_count), kind(_kind) {
  llvmType = kind == VectorKind::fixed ? (llvm::Type*)llvm::FixedVectorType::get(subType->getLLVMType(), count)
                                       : llvm::ScalableVectorType::get(subType->getLLVMType(), count);
}

VectorType* VectorType::create(IR::QatType* subType, usize count, VectorKind kind, IR::Context* ctx) {
  for (auto typ : allQatTypes) {
    if (typ->typeKind() == TypeKind::vector) {
      if (((VectorType*)typ)->subType->isSame(subType) && (((VectorType*)typ)->count == count) &&
          (((VectorType*)typ)->kind == kind)) {
        return (VectorType*)typ;
      }
    }
  }
  return new VectorType(subType, count, kind, ctx);
}

String VectorType::toString() const {
  return String("vec:[") + ((kind == VectorKind::scalable) ? "?, " : "") + std::to_string(count) + ", " +
         subType->toString() + "]";
}

} // namespace qat::IR