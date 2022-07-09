#include "./float.hpp"

namespace qat {
namespace IR {

FloatType::FloatType(const FloatTypeKind _kind) : kind(_kind) {}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

} // namespace IR
} // namespace qat