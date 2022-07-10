#include "./integer.hpp"

namespace qat {
namespace IR {

IntegerType::IntegerType(const unsigned int _bitWidth) : bitWidth(_bitWidth) {}

bool IntegerType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

unsigned IntegerType::getBitwidth() const { return bitWidth; }

std::string IntegerType::toString() const {
  return "i" + std::to_string(bitWidth);
}

} // namespace IR
} // namespace qat