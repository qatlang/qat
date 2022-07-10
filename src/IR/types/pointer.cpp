#include "./pointer.hpp"

namespace qat {
namespace IR {

PointerType::PointerType(QatType *_type) : subType(_type) {}

QatType *PointerType::getSubType() const { return subType; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

std::string PointerType::toString() const {
  return "#[" + subType->toString() + "]";
}

} // namespace IR
} // namespace qat