#include "./reference.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace IR {

ReferenceType::ReferenceType(QatType *_type) : subType(_type) {}

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

} // namespace IR
} // namespace qat