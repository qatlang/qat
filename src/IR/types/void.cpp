#include "./void.hpp"

namespace qat {
namespace IR {

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

} // namespace IR
} // namespace qat