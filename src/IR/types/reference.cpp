#include "./reference.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace IR {

ReferenceType::ReferenceType(QatType *_type) : subType(_type) {
  llvmType = llvm::PointerType::get(_type->getLLVMType(), 0U);
}

TypeKind ReferenceType::typeKind() { return TypeKind::reference; }

} // namespace IR
} // namespace qat