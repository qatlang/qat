#include "./integer.hpp"

namespace qat::IR {

IntegerType::IntegerType(const u32 _bitWidth) : bitWidth(_bitWidth) {}

bool IntegerType::isBitWidth(const u32 width) const {
  return bitWidth == width;
}

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

u64 IntegerType::getBitwidth() const { return bitWidth; }

String IntegerType::toString() const { return "i" + std::to_string(bitWidth); }

} // namespace qat::IR