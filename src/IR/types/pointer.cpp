#include "./pointer.hpp"

namespace qat::IR {

PointerType::PointerType(QatType *_type) : subType(_type) {}

QatType *PointerType::getSubType() const { return subType; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return "#[" + subType->toString() + "]";
}

} // namespace qat::IR