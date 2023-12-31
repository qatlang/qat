#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

PointerType::PointerType(QatType* _type, bool _isSubtypeVar, PtrOwnType ownTy, bool _isNonNull,
                         Maybe<QatType*> _ownTyTy, bool _isMultiPtr, FileRange _fileRange)
    : QatType(std::move(_fileRange)), type(_type), ownTyp(ownTy), ownerTyTy(_ownTyTy), isMultiPtr(_isMultiPtr),
      isSubtypeVar(_isSubtypeVar), isNonNullable(_isNonNull) {}

Maybe<usize> PointerType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      isMultiPtr
          ? llvm::cast<llvm::Type>(llvm::StructType::create(
                {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()),
                 llvm::Type::getInt64Ty(ctx->llctx)}))
          : llvm::cast<llvm::Type>(
                llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()))));
}

String PointerType::pointerOwnerToString() const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return "type";
    case PtrOwnType::typeParent:
      return "typeParent";
    case PtrOwnType::function:
      return "function";
    case PtrOwnType::anonymous:
      return "anonymous";
    case PtrOwnType::heap:
      return "heap";
    case PtrOwnType::region:
      return "region";
  }
}

IR::PointerOwner PointerType::getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return IR::PointerOwner::OfType(ownerVal.value());
    case PtrOwnType::typeParent:
      return IR::PointerOwner::OfParentInstance(ctx->getActiveType());
    case PtrOwnType::anonymous:
      return IR::PointerOwner::OfAnonymous();
    case PtrOwnType::heap:
      return IR::PointerOwner::OfHeap();
    case PtrOwnType::function:
      return IR::PointerOwner::OfParentFunction(ctx->getActiveFunction());
    case PtrOwnType::region:
      return IR::PointerOwner::OfRegion(ownerVal.value()->asRegion());
  }
}

IR::QatType* PointerType::emit(IR::Context* ctx) {
  if (ownTyp == PtrOwnType::function) {
    if (!ctx->getActiveFunction()) {
      ctx->Error("This pointer type is not inside a function and hence cannot have function ownership", fileRange);
    }
  } else if (ownTyp == PtrOwnType::typeParent) {
    if (!ctx->hasActiveType()) {
      ctx->Error("This pointer type is not inside a type and hence the pointer "
                 "cannot be owned by any type",
                 fileRange);
    }
  }
  Maybe<IR::QatType*> ownerVal;
  if (ownTyp == PtrOwnType::type) {
    if (!ownerTyTy) {
      ctx->Error("Expected a type to be provided", fileRange);
    }
    auto* typVal = ownerTyTy.value()->emit(ctx);
    if (typVal->isRegion()) {
      ctx->Error("The type provided is a region and hence pointer ownership has to be " +
                     ctx->highlightError("'region"),
                 fileRange);
    }
    ownerVal = typVal;
  } else if (ownTyp == PtrOwnType::region) {
    if (!ownerTyTy) {
      ctx->Error("Expected a region to be provided", fileRange);
    }
    auto* regTy = ownerTyTy.value()->emit(ctx);
    if (!regTy->isRegion()) {
      ctx->Error("The provided type is not a region type and hence pointer "
                 "owner cannot be " +
                     ctx->highlightError("region"),
                 fileRange);
    }
    ownerVal = regTy;
  }
  return IR::PointerType::get(isSubtypeVar, type->emit(ctx), isNonNullable, getPointerOwner(ctx, ownerVal), isMultiPtr,
                              ctx);
}

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

Json PointerType::toJson() const {
  return Json()
      ._("typeKind", "pointer")
      ._("isMulti", isMultiPtr)
      ._("isSubtypeVariable", isSubtypeVar)
      ._("subType", type->toJson())
      ._("ownerKind", pointerOwnerToString())
      ._("hasOwnerType", ownerTyTy.has_value())
      ._("ownerType", ownerTyTy.has_value() ? ownerTyTy.value()->toJson() : Json())
      ._("fileRange", fileRange);
}

String PointerType::toString() const {
  Maybe<String> ownerStr;
  switch (ownTyp) {
    case PtrOwnType::type: {
      ownerStr = String("type(") + ownerTyTy.value()->toString() + ")";
      break;
    }
    case PtrOwnType::typeParent: {
      ownerStr = "''";
      break;
    }
    case PtrOwnType::heap: {
      ownerStr = "heap";
      break;
    }
    case PtrOwnType::function: {
      ownerStr = "own";
      break;
    }
    case PtrOwnType::region: {
      ownerStr = String("region(") + ownerTyTy.value()->toString() + ")";
      break;
    }
    default:;
  }
  return (isMultiPtr ? "multiptr:[" : "ptr:[") + String(isSubtypeVar ? "var " : "") + type->toString() +
         (ownerStr.has_value() ? (", " + ownerStr.value()) : "") + "]";
}

} // namespace qat::ast
