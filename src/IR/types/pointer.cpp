#include "./pointer.hpp"
#include "../../show.hpp"
#include "qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace IR {

PointerType::PointerType(QatType *_type) : subType(_type) {
  llvmType = llvm::PointerType::get(_type->getLLVMType(), 0U);
}

QatType *PointerType::getSubType() const { return subType; }

TypeKind PointerType::typeKind() { return TypeKind::pointer; }

} // namespace IR
} // namespace qat