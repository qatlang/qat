#include "./unsigned.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

UnsignedType::UnsignedType(u64 _bitWidth, llvm::LLVMContext &ctx)
    : bitWidth(_bitWidth) {
  llvmType = llvm::IntegerType::get(ctx, bitWidth);
}

UnsignedType *UnsignedType::get(u64 bits, llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBitWidth(bits)) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(bits, ctx);
}

u64 UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(u64 width) const { return bitWidth == width; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

String UnsignedType::toString() const { return "u" + std::to_string(bitWidth); }

nuo::Json UnsignedType::toJson() const {
  return nuo::Json()
      ._("type", "unsigned")
      ._("bitWidth", std::to_string(bitWidth));
}

} // namespace qat::IR