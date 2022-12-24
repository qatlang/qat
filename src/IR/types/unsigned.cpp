#include "./unsigned.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

UnsignedType::UnsignedType(u64 _bitWidth, llvm::LLVMContext& ctx, bool _isBool) : bitWidth(_bitWidth), isBool(_isBool) {
  llvmType = llvm::IntegerType::get(ctx, bitWidth);
}

UnsignedType* UnsignedType::get(u64 bits, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBitWidth(bits) && !typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(bits, ctx, false);
}

UnsignedType* UnsignedType::getBool(llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(1u, ctx, true);
}

u64 UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(u64 width) const { return bitWidth == width; }

bool UnsignedType::isBoolean() const { return isBool; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

String UnsignedType::toString() const { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

Json UnsignedType::toJson() const { return Json()._("type", "unsigned")._("bitWidth", std::to_string(bitWidth)); }

} // namespace qat::IR