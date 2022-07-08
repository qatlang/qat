#include "./array.hpp"

namespace qat {
namespace IR {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length)
    : element_type(_element_type), length(_length) {}

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

} // namespace IR
} // namespace qat