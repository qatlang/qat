#include "./pointer.hpp"
#include "../../memory_tracker.hpp"
#include "../function.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat::IR {

PointerOwner PointerOwner::OfHeap() { return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::heap}; }

PointerOwner PointerOwner::OfType(QatType* type) {
  return PointerOwner{.owner = (void*)type, .ownerTy = PointerOwnerType::type};
}

PointerOwner PointerOwner::OfAnonymous() {
  return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anonymous};
}

PointerOwner PointerOwner::OfFunction(Function* fun) {
  return PointerOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::function};
}

PointerOwner PointerOwner::OfRegion() { return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::region}; }

QatType* PointerOwner::ownerAsType() const { return (QatType*)owner; }

Function* PointerOwner::ownerAsFunction() const { return (Function*)owner; }

bool PointerOwner::isType() const { return ownerTy == PointerOwnerType::type; }

bool PointerOwner::isAnonymous() const { return ownerTy == PointerOwnerType::anonymous; }

bool PointerOwner::isRegion() const { return ownerTy == PointerOwnerType::region; }

bool PointerOwner::isHeap() const { return ownerTy == PointerOwnerType::heap; }

bool PointerOwner::isFunction() const { return ownerTy == PointerOwnerType::function; }

bool PointerOwner::isSame(const PointerOwner& other) const {
  if (ownerTy == other.ownerTy) {
    switch (ownerTy) {
      case PointerOwnerType::anonymous:
      case PointerOwnerType::heap:
      case PointerOwnerType::region: // FIXME - Change this once region is implemented!!!
        return true;
      case PointerOwnerType::type:
        return ownerAsType()->isSame(other.ownerAsType());
      case PointerOwnerType::function:
        return ownerAsFunction()->getID() == other.ownerAsFunction()->getID();
    }
  } else {
    return false;
  }
}

String PointerOwner::toString() const {
  switch (ownerTy) {
    case PointerOwnerType::region:
      return "'region";
    case PointerOwnerType::heap:
      return "'heap";
    case PointerOwnerType::anonymous:
      return "?";
    case PointerOwnerType::type:
      return "'type(" + ownerAsType()->toString() + ")";
    case PointerOwnerType::function:
      return "";
  }
}

PointerType::PointerType(bool _isSubtypeVariable, QatType* _type, PointerOwner _owner, llvm::LLVMContext& ctx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner) {
  llvmType = llvm::PointerType::get(subType->getLLVMType()->isVoidTy()
                                        ? llvm::Type::getInt8Ty(subType->getLLVMType()->getContext())
                                        : subType->getLLVMType(),
                                    0U);
}

PointerType* PointerType::get(bool _isSubtypeVariable, QatType* _type, PointerOwner _owner, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isPointer()) {
      if (typ->asPointer()->getSubType()->isSame(_type) &&
          (typ->asPointer()->isSubtypeVariable() == _isSubtypeVariable) &&
          typ->asPointer()->getOwner().isSame(_owner)) {
        return typ->asPointer();
      }
    }
  }
  return new PointerType(_isSubtypeVariable, _type, _owner, ctx);
}

bool PointerType::isSubtypeVariable() const { return isSubtypeVar; }

QatType* PointerType::getSubType() const { return subType; }

PointerOwner PointerType::getOwner() const { return owner; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return "#[" + String(isSubtypeVariable() ? "var " : "") + subType->toString() + " " + owner.toString() + "]";
}

Json PointerType::toJson() const {
  return Json()._("type", "pointer")._("subtype", subType->getID())._("isSubtypeVariable", isSubtypeVariable());
}

} // namespace qat::IR