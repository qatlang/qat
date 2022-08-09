#include "./string_slice.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

StringSliceType::StringSliceType(llvm::LLVMContext &ctx) {
  if (llvm::StructType::getTypeByName(ctx, "str")) {
    llvmType = llvm::StructType::getTypeByName(ctx, "str");
  } else {
    llvmType = llvm::StructType::create(
        {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx), 0U),
         llvm::Type::getInt64Ty(ctx)},
        "str");
  }
}

StringSliceType *StringSliceType::get(llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->typeKind() == TypeKind::stringSlice) {
      return (StringSliceType *)typ;
    }
  }
  return new StringSliceType(ctx);
}

TypeKind StringSliceType::typeKind() const { return TypeKind::stringSlice; }

String StringSliceType::toString() const { return "str"; }

nuo::Json StringSliceType::toJson() const {
  return nuo::Json()._("type", "str");
}

} // namespace qat::IR