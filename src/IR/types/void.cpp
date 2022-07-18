#include "./void.hpp"

namespace qat::IR {

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

std::string VoidType::toString() const { return "void"; }

} // namespace qat::IR