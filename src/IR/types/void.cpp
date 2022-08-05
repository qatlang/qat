#include "./void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

VoidType::VoidType(llvm::LLVMContext &ctx) {
  llvmType = llvm::Type::getVoidTy(ctx);
}

VoidType *VoidType::get(llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->typeKind() == TypeKind::Void) {
      return (VoidType *)typ;
    }
  }
  return new VoidType(ctx);
}

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

String VoidType::toString() const { return "void"; }

nuo::Json VoidType::toJson() const { return nuo::Json()._("typeKind", "void"); }

} // namespace qat::IR