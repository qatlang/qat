#include "./unsigned.hpp"

namespace qat {
namespace IR {

UnsignedType::UnsignedType(const unsigned int _bitWidth)
    : bitWidth(_bitWidth) {}

unsigned UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

} // namespace IR
} // namespace qat