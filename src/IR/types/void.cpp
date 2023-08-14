#include "./void.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

VoidType::VoidType(llvm::LLVMContext& llctx) { llvmType = llvm::Type::getVoidTy(llctx); }

VoidType* VoidType::get(llvm::LLVMContext& llctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::Void) {
      return (VoidType*)typ;
    }
  }
  return new VoidType(llctx);
}

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

String VoidType::toString() const { return "void"; }

} // namespace qat::IR