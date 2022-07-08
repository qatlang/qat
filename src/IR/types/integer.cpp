#include "./integer.hpp"

namespace qat {
namespace IR {

IntegerType::IntegerType(llvm::LLVMContext &ctx, const unsigned int _bitWidth)
    : bitWidth(_bitWidth) {
  llvmType = llvm::Type::getIntNTy(ctx, _bitWidth);
}

bool IntegerType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

unsigned IntegerType::getBitwidth() const { return bitWidth; }

} // namespace IR
} // namespace qat