#include "./maybe.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

MaybeType::MaybeType(QatType* _subType, llvm::LLVMContext& ctx) : subTy(_subType) {
  llvmType = llvm::StructType::create(ctx, {llvm::Type::getInt1Ty(ctx), subTy->getLLVMType()},
                                      "maybe " + subTy->toString(), false);
}

MaybeType* MaybeType::get(QatType* subTy, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isMaybe()) {
      if (typ->asMaybe()->getSubType()->isSame(subTy)) {
        return typ->asMaybe();
      }
    }
  }
  return new MaybeType(subTy, ctx);
}

QatType* MaybeType::getSubType() const { return subTy; }

String MaybeType::toString() const { return "maybe " + subTy->toString(); }

} // namespace qat::IR