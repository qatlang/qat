#include "./void.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

VoidType::VoidType(llvm::LLVMContext& llctx) {
  llvmType    = llvm::Type::getVoidTy(llctx);
  linkingName = "qat'void";
}

VoidType* VoidType::get(llvm::LLVMContext& llctx) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::Void) {
      return (VoidType*)typ;
    }
  }
  return new VoidType(llctx);
}

TypeKind VoidType::type_kind() const { return TypeKind::Void; }

String VoidType::to_string() const { return "void"; }

} // namespace qat::ir