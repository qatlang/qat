#include "./array.hpp"
#include "qat_type.hpp"

namespace qat {
namespace IR {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length)
    : element_type(_element_type), length(_length) {}

QatType *ArrayType::getElementType() { return element_type; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

} // namespace IR
} // namespace qat