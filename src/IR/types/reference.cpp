#include "./reference.hpp"

namespace qat {
namespace IR {

ReferenceType::ReferenceType(QatType *_type) : subType(_type) {}

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

std::string ReferenceType::toString() const {
  return "@" + subType->toString();
}

} // namespace IR
} // namespace qat