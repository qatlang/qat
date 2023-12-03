#include "./reference.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ReferenceType::ReferenceType(bool _isSubtypeVariable, QatType* _type, IR::Context* ctx)
    : subType(_type), isSubVariable(_isSubtypeVariable) {
  if (subType->isTypeSized()) {
    llvmType = llvm::PointerType::get(subType->getLLVMType(), ctx->dataLayout->getProgramAddressSpace());
  } else {
    llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace());
  }
  linkingName = "qat'@" + String(isSubVariable ? "var[" : "[") + subType->getNameForLinking() + "]";
}

ReferenceType* ReferenceType::get(bool _isSubtypeVariable, QatType* _subtype, IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->isReference()) {
      if (typ->asReference()->getSubType()->isSame(_subtype) &&
          (typ->asReference()->isSubtypeVariable() == _isSubtypeVariable)) {
        return typ->asReference();
      }
    }
  }
  return new ReferenceType(_isSubtypeVariable, _subtype, ctx);
}

QatType* ReferenceType::getSubType() const { return subType; }

bool ReferenceType::isSubtypeVariable() const { return isSubVariable; }

bool ReferenceType::isTypeSized() const { return true; }

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

String ReferenceType::toString() const {
  return "@" + String(isSubVariable ? "var[" : "[") + subType->toString() + "]";
}

} // namespace qat::IR