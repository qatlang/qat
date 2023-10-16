#include "./pointer.hpp"
#include "../../memory_tracker.hpp"
#include "../function.hpp"
#include "region.hpp"
#include "llvm/IR/DerivedTypes.h"
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

PointerOwner PointerOwner::OfParentFunction(Function* fun) {
  return PointerOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::parentFunction};
}

PointerOwner PointerOwner::OfParentType(QatType* typ) {
  return PointerOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentType};
}

PointerOwner PointerOwner::OfRegion(Region* region) {
  return PointerOwner{.owner = region, .ownerTy = PointerOwnerType::region};
}

QatType* PointerOwner::ownerAsType() const { return (QatType*)owner; }

Region* PointerOwner::ownerAsRegion() const { return ((QatType*)owner)->asRegion(); }

Function* PointerOwner::ownerAsParentFunction() const { return (Function*)owner; }

QatType* PointerOwner::ownerAsParentType() const { return (QatType*)owner; }

bool PointerOwner::isType() const { return ownerTy == PointerOwnerType::type; }

bool PointerOwner::isAnonymous() const { return ownerTy == PointerOwnerType::anonymous; }

bool PointerOwner::isRegion() const { return ownerTy == PointerOwnerType::region; }

bool PointerOwner::isHeap() const { return ownerTy == PointerOwnerType::heap; }

bool PointerOwner::isParentFunction() const { return ownerTy == PointerOwnerType::parentFunction; }

bool PointerOwner::isParentType() const { return ownerTy == PointerOwnerType::parentType; }

bool PointerOwner::isSame(const PointerOwner& other) const {
  if (ownerTy == other.ownerTy) {
    switch (ownerTy) {
      case PointerOwnerType::anonymous:
      case PointerOwnerType::heap:
        return true;
      case PointerOwnerType::region:
        return ownerAsRegion()->isSame(other.ownerAsRegion());
      case PointerOwnerType::type:
        return ownerAsType()->isSame(other.ownerAsType());
      case PointerOwnerType::parentFunction:
        return ownerAsParentFunction()->getID() == other.ownerAsParentFunction()->getID();
      case PointerOwnerType::parentType:
        return ownerAsParentType()->getID() == other.ownerAsParentType()->getID();
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
      return "";
    case PointerOwnerType::type:
      return "'type(" + ownerAsType()->toString() + ")";
    case PointerOwnerType::parentType:
    case PointerOwnerType::parentFunction:
      return "'own";
  }
}

PointerType::PointerType(bool _isSubtypeVariable, QatType* _type, PointerOwner _owner, bool _hasMulti, IR::Context* ctx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti) {
  if (_hasMulti) {
    if (llvm::StructType::getTypeByName(ctx->llctx, "multiptr:[" + subType->toString() + "]")) {
      llvmType = llvm::StructType::getTypeByName(ctx->llctx, "multiptr:[" + subType->toString() + "]");
    } else {
      llvmType = llvm::StructType::create(
          {llvm::PointerType::get(subType->getLLVMType()->isVoidTy() ? llvm::Type::getInt8Ty(ctx->llctx)
                                                                     : subType->getLLVMType(),
                                  ctx->dataLayout->getProgramAddressSpace()),
           llvm::Type::getInt64Ty(ctx->llctx)},
          "multiptr:[" + subType->toString() + "]");
    }
  } else {
    llvmType = llvm::PointerType::get(
        subType->getLLVMType()->isVoidTy() ? llvm::Type::getInt8Ty(ctx->llctx) : subType->getLLVMType(), 0U);
  }
}

PointerType* PointerType::get(bool _isSubtypeVariable, QatType* _type, PointerOwner _owner, bool _hasMulti,
                              IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->isPointer()) {
      if (typ->asPointer()->getSubType()->isSame(_type) &&
          (typ->asPointer()->isSubtypeVariable() == _isSubtypeVariable) &&
          typ->asPointer()->getOwner().isSame(_owner) && (typ->asPointer()->isMulti() == _hasMulti)) {
        return typ->asPointer();
      }
    }
  }
  return new PointerType(_isSubtypeVariable, _type, _owner, _hasMulti, ctx);
}

bool PointerType::isSubtypeVariable() const { return isSubtypeVar; }

bool PointerType::isTypeSized() const { return true; }

bool PointerType::isMulti() const { return hasMulti; }

QatType* PointerType::getSubType() const { return subType; }

PointerOwner PointerType::getOwner() const { return owner; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return String(isMulti() ? "multiptr:[" : "ptr:[") + String(isSubtypeVariable() ? "var " : "") + subType->toString() +
         " " + owner.toString() + "]";
}

} // namespace qat::IR
