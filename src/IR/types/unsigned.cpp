#include "./unsigned.hpp"

namespace qat {
namespace IR {

UnsignedType::UnsignedType(llvm::LLVMContext &ctx, const unsigned int _bitWidth)
    : bitWidth(_bitWidth) {
  llvmType = llvm::Type::getIntNTy(ctx, _bitWidth);
}

bool UnsignedType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

} // namespace IR
} // namespace qat