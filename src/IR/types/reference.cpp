#include "./reference.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ReferenceType::ReferenceType(bool _isSubtypeVariable, QatType* _type,
                             llvm::LLVMContext& ___) // NOLINT(misc-unused-parameters)
    : subType(_type), isSubVariable(_isSubtypeVariable) {
  llvmType = llvm::PointerType::get(subType->getLLVMType(), 0U);
}

ReferenceType* ReferenceType::get(bool _isSubtypeVariable, QatType* _subtype, llvm::LLVMContext& ctx) {
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

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

String ReferenceType::toString() const { return "@" + String(isSubVariable ? " var " : "") + subType->toString(); }

Json ReferenceType::toJson() const {
  return Json()._("type", "reference")._("subType", subType->getID())._("isSubtypeVariable", isSubVariable);
}

} // namespace qat::IR