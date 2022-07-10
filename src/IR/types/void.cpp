#include "./void.hpp"

namespace qat {
namespace IR {

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

std::string VoidType::toString() const { return "void"; }

} // namespace IR
} // namespace qat