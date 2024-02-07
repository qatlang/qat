#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

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
    case PtrOwnType::anyRegion:
      return "anyRegion";
  }
}

IR::PointerOwner PointerType::getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return IR::PointerOwner::OfType(ownerVal.value());
    case PtrOwnType::typeParent: {
      if (ctx->hasActiveType() || ctx->has_active_done_skill()) {
        return IR::PointerOwner::OfParentInstance(ctx->hasActiveType() ? ctx->getActiveType()
                                                                       : ctx->get_active_done_skill()->getType());
      } else {
        ctx->Error("No parent type or skill found", fileRange);
      }
    }
    case PtrOwnType::anonymous:
      return IR::PointerOwner::OfAnonymous();
    case PtrOwnType::heap:
      return IR::PointerOwner::OfHeap();
    case PtrOwnType::function:
      return IR::PointerOwner::OfParentFunction(ctx->getActiveFunction());
    case PtrOwnType::region:
      return IR::PointerOwner::OfRegion(ownerVal.value()->asRegion());
    case PtrOwnType::anyRegion:
      return IR::PointerOwner::OfAnyRegion();
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
    if (ownerTyTy) {
      auto* regTy = ownerTyTy.value()->emit(ctx);
      if (!regTy->isRegion()) {
        ctx->Error("The provided type is not a region type and hence pointer "
                   "owner cannot be " +
                       ctx->highlightError("region"),
                   fileRange);
      }
      ownerVal = regTy;
    }
  }
  auto subTy = type->emit(ctx);
  if (isSubtypeVar && subTy->isFunction()) {
    ctx->Error(
        "The subtype of this pointer type is a function type, and hence variability cannot be specified here. Please remove " +
            ctx->highlightError("var"),
        fileRange);
  }
  return IR::PointerType::get(isSubtypeVar, subTy, isNonNullable, getPointerOwner(ctx, ownerVal), isMultiPtr, ctx);
}

AstTypeKind PointerType::typeKind() const { return AstTypeKind::POINTER; }

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
    case PtrOwnType::anyRegion: {
      ownerStr = "region";
      break;
    }
    default:;
  }
  return (isMultiPtr ? "multiptr:[" : "ptr:[") + String(isSubtypeVar ? "var " : "") + type->toString() +
         (ownerStr.has_value() ? (", " + ownerStr.value()) : "") + "]";
}

} // namespace qat::ast
