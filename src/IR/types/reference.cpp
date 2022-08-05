#include "./reference.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ReferenceType::ReferenceType(
    QatType *_type, llvm::LLVMContext &___) // NOLINT(misc-unused-parameters)
    : subType(_type) {
  llvmType = llvm::PointerType::get(subType->getLLVMType(), 0U);
}

ReferenceType *ReferenceType::get(QatType *_subtype, llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->isReference()) {
      if (typ->asReference()->getSubType()->isSame(_subtype)) {
        return typ->asReference();
      }
    }
  }
  return new ReferenceType(_subtype, ctx);
}

QatType *ReferenceType::getSubType() const { return subType; }

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

String ReferenceType::toString() const { return "@" + subType->toString(); }

nuo::Json ReferenceType::toJson() const {
  return nuo::Json()._("type", "reference")._("subType", subType->getID());
}

} // namespace qat::IR