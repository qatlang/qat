#include "./pointer.hpp"
#include "../function.hpp"
#include "region.hpp"
#include "llvm/IR/Constants.h"
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

PointerOwner PointerOwner::OfParentInstance(QatType* typ) {
  return PointerOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentInstance};
}

PointerOwner PointerOwner::OfRegion(Region* region) {
  return PointerOwner{.owner = region, .ownerTy = PointerOwnerType::region};
}

PointerOwner PointerOwner::OfAnyRegion() {
  return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anyRegion};
}

bool PointerOwner::isSame(const PointerOwner& other) const {
  if (ownerTy == other.ownerTy) {
    switch (ownerTy) {
      case PointerOwnerType::anonymous:
      case PointerOwnerType::Static:
      case PointerOwnerType::heap:
      case PointerOwnerType::anyRegion:
        return true;
      case PointerOwnerType::region:
        return ownerAsRegion()->isSame(other.ownerAsRegion());
      case PointerOwnerType::type:
        return ownerAsType()->isSame(other.ownerAsType());
      case PointerOwnerType::parentFunction:
        return ownerAsParentFunction()->getID() == other.ownerAsParentFunction()->getID();
      case PointerOwnerType::parentInstance:
        return ownerAsParentType()->getID() == other.ownerAsParentType()->getID();
    }
  } else {
    return false;
  }
}

String PointerOwner::toString() const {
  switch (ownerTy) {
    case PointerOwnerType::anyRegion:
      return "region";
    case PointerOwnerType::region:
      return "region(" + ownerAsRegion()->toString() + ")";
    case PointerOwnerType::heap:
      return "heap";
    case PointerOwnerType::anonymous:
      return "";
    case PointerOwnerType::type:
      return "type(" + ownerAsType()->toString() + ")";
    case PointerOwnerType::parentInstance:
      return "''(" + ownerAsParentType()->toString() + ")";
    case PointerOwnerType::parentFunction:
      return "own(" + ownerAsParentFunction()->getFullName() + ")";
    case PointerOwnerType::Static:
      return "static";
  }
}

PointerType::PointerType(bool _isSubtypeVariable, QatType* _type, bool _nonNullable, PointerOwner _owner,
                         bool _hasMulti, IR::Context* ctx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable) {
  if (_hasMulti) {
    linkingName = (nonNullable ? "qat'multiptr![" : "qat'multiptr:[") + String(isSubtypeVar ? "var " : "") +
                  subType->getNameForLinking() + (owner.isAnonymous() ? "" : ",") + owner.toString() + "]";
    if (llvm::StructType::getTypeByName(ctx->llctx, linkingName)) {
      llvmType = llvm::StructType::getTypeByName(ctx->llctx, linkingName);
    } else {
      llvmType = llvm::StructType::create(
          {llvm::PointerType::get(llvm::PointerType::isValidElementType(subType->getLLVMType())
                                      ? subType->getLLVMType()
                                      : llvm::Type::getInt8Ty(ctx->llctx),
                                  ctx->dataLayout->getProgramAddressSpace()),
           llvm::Type::getIntNTy(ctx->llctx, ctx->clangTargetInfo->getTypeWidth(ctx->clangTargetInfo->getSizeType()))},
          linkingName);
    }
  } else {
    linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
                  subType->getNameForLinking() + (owner.isAnonymous() ? "" : ",") + owner.toString() + "]";
    llvmType = llvm::PointerType::get(llvm::PointerType::isValidElementType(subType->getLLVMType())
                                          ? subType->getLLVMType()
                                          : llvm::Type::getInt8Ty(ctx->llctx),
                                      ctx->dataLayout->getProgramAddressSpace());
  }
}

PointerType* PointerType::get(bool _isSubtypeVariable, QatType* _type, bool _nonNullable, PointerOwner _owner,
                              bool _hasMulti, IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isPointer()) {
      if (typ->asPointer()->getSubType()->isSame(_type) &&
          (typ->asPointer()->isSubtypeVariable() == _isSubtypeVariable) &&
          typ->asPointer()->getOwner().isSame(_owner) && (typ->asPointer()->isMulti() == _hasMulti) &&
          (typ->asPointer()->nonNullable == _nonNullable)) {
        return typ->asPointer();
      }
    }
  }
  return new PointerType(_isSubtypeVariable, _type, _nonNullable, _owner, _hasMulti, ctx);
}

bool PointerType::isSubtypeVariable() const { return isSubtypeVar; }

bool PointerType::isTypeSized() const { return true; }

bool PointerType::hasPrerunDefaultValue() const { return !nonNullable; }

IR::PrerunValue* PointerType::getPrerunDefaultValue(IR::Context* ctx) {
  if (hasPrerunDefaultValue()) {
    if (isMulti()) {
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(
              llvm::cast<llvm::StructType>(getLLVMType()),
              {llvm::ConstantPointerNull::get(llvm::PointerType::get(
                  getSubType()->isVoid() ? llvm::Type::getInt8Ty(ctx->llctx) : getSubType()->getLLVMType(),
                  ctx->dataLayout.value().getProgramAddressSpace()))}),
          this);
    } else {
      return new IR::PrerunValue(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(getLLVMType())), this);
    }
  } else {
    ctx->Error("Type " + ctx->highlightError(toString()) + " do not have a default value", None);
    return nullptr;
  }
}

bool PointerType::isTriviallyCopyable() const { return true; }

bool PointerType::isTriviallyMovable() const { return !nonNullable; }

bool PointerType::isMulti() const { return hasMulti; }

bool PointerType::isNullable() const { return !nonNullable; }

bool PointerType::isNonNullable() const { return nonNullable; }

QatType* PointerType::getSubType() const { return subType; }

PointerOwner PointerType::getOwner() const { return owner; }

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

String PointerType::toString() const {
  return String(isMulti() ? (nonNullable ? "multiptr![" : "multiptr:[") : (nonNullable ? "ptr![" : "ptr:[")) +
         String(isSubtypeVariable() ? "var " : "") + subType->toString() + (owner.isAnonymous() ? "" : " ") +
         owner.toString() + "]";
}

} // namespace qat::IR
