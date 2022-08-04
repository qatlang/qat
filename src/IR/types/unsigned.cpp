#include "./unsigned.hpp"

namespace qat::IR {

UnsignedType::UnsignedType(const u32 _bitWidth) : bitWidth(_bitWidth) {}

u64 UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(const u32 width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

String UnsignedType::toString() const { return "u" + std::to_string(bitWidth); }

} // namespace qat::IR