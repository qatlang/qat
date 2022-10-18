#include "./future.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

FutureType::FutureType(QatType* _subType, llvm::LLVMContext& ctx) : subTy(_subType) {
  llvmType = llvm::StructType::create(
      ctx, {llvm::Type::getInt8Ty(ctx)->getPointerTo(), llvm::Type::getInt1Ty(ctx), subTy->getLLVMType()},
      "future " + subTy->toString(), false);
}

FutureType* FutureType::get(QatType* subType, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isFuture()) {
      if (typ->asFuture()->getSubType()->isSame(subType)) {
        return typ->asFuture();
      }
    }
  }
  return new FutureType(subType, ctx);
}

QatType* FutureType::getSubType() const { return subTy; }

String FutureType::toString() const { return "future " + subTy->toString(); }

TypeKind FutureType::typeKind() const { return TypeKind::future; }

} // namespace qat::IR