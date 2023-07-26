#include "./cstring.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

CStringType::CStringType(llvm::LLVMContext& ctx) { llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(ctx), 0U); }

CStringType* CStringType::get(llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::cstring) {
      return (CStringType*)typ;
    }
  }
  return new CStringType(ctx);
}

TypeKind CStringType::typeKind() const { return TypeKind::cstring; }

String CStringType::toString() const { return "cstring"; }

} // namespace qat::IR