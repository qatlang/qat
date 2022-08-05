#include "./pointer.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat::IR {

PointerType::PointerType(QatType *_type, llvm::LLVMContext &ctx)
    : subType(_type) {
  llvmType = llvm::PointerType::get(subType->getLLVMType(), 0U);
}

PointerType *PointerType::get(QatType *_type, llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->isPointer()) {
      if (typ->asPointer()->getSubType()->isSame(_type)) {
        return typ->asPointer();
      }
    }
  }
  return new PointerType(_type, ctx);
}

QatType *PointerType::getSubType() const { return subType; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return "#[" + subType->toString() + "]";
}

nuo::Json PointerType::toJson() const {
  return nuo::Json()._("type", "pointer")._("subtype", subType->getID());
}

} // namespace qat::IR