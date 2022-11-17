#include "./pointer.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat::IR {

PointerType::PointerType(bool _isSubtypeVariable, QatType* _type, llvm::LLVMContext& ctx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable) {
  llvmType = llvm::PointerType::get(subType->getLLVMType()->isVoidTy()
                                        ? llvm::Type::getInt8Ty(subType->getLLVMType()->getContext())
                                        : subType->getLLVMType(),
                                    0U);
}

PointerType* PointerType::get(bool _isSubtypeVariable, QatType* _type, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isPointer()) {
      if (typ->asPointer()->getSubType()->isSame(_type) &&
          typ->asPointer()->isSubtypeVariable() == _isSubtypeVariable) {
        return typ->asPointer();
      }
    }
  }
  return new PointerType(_isSubtypeVariable, _type, ctx);
}

bool PointerType::isSubtypeVariable() const { return isSubtypeVar; }

QatType* PointerType::getSubType() const { return subType; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return "#[" + String(isSubtypeVariable() ? "var " : "") + subType->toString() + "]";
}

Json PointerType::toJson() const {
  return Json()._("type", "pointer")._("subtype", subType->getID())._("isSubtypeVariable", isSubtypeVariable());
}

} // namespace qat::IR