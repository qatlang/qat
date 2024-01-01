#include "./void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

VoidType::VoidType(llvm::LLVMContext& llctx) {
  llvmType    = llvm::Type::getVoidTy(llctx);
  linkingName = "qat'void";
}

VoidType* VoidType::get(llvm::LLVMContext& llctx) {
  for (auto* typ : allQatTypes) {
    if (typ->typeKind() == TypeKind::Void) {
      return (VoidType*)typ;
    }
  }
  return new VoidType(llctx);
}

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

String VoidType::toString() const { return "void"; }

} // namespace qat::IR