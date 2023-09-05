#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

PointerType::PointerType(QatType* _type, bool _variable, PtrOwnType ownTy, Maybe<QatType*> _ownTyTy, bool _isMultiPtr,
                         FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), type(_type), ownTyp(ownTy), ownerTyTy(_ownTyTy),
      isMultiPtr(_isMultiPtr) {}

Maybe<usize> PointerType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      isMultiPtr
          ? llvm::cast<llvm::Type>(llvm::StructType::create(
                {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()),
                 llvm::Type::getInt64Ty(ctx->llctx)}))
          : llvm::cast<llvm::Type>(
                llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()))));
}

IR::PointerOwner PointerType::getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return IR::PointerOwner::OfType(ownerVal.value());
    case PtrOwnType::typeParent:
      return IR::PointerOwner::OfType(ctx->activeType);
    case PtrOwnType::anonymous:
      return IR::PointerOwner::OfAnonymous();
    case PtrOwnType::heap:
      return IR::PointerOwner::OfHeap();
    case PtrOwnType::parent: {
      if (ctx->fn) {
        return IR::PointerOwner::OfFunction(ctx->fn);
      } else {
        return IR::PointerOwner::OfType(ctx->activeType);
      }
    }
    case PtrOwnType::region:
      return IR::PointerOwner::OfRegion(ownerVal.value()->asRegion());
  }
}

IR::QatType* PointerType::emit(IR::Context* ctx) {
  if (ownTyp == PtrOwnType::parent) {
    if (!ctx->fn && !ctx->activeType) {
      ctx->Error("This pointer type is not inside a function or type and hence "
                 "the pointer cannot have parent ownership",
                 fileRange);
    }
  } else if (ownTyp == PtrOwnType::typeParent) {
    if (!ctx->activeType) {
      ctx->Error("This pointer type is not inside a type and hence the pointer "
                 "cannot be owned by type",
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
      ctx->Error("The type provided is a region and hence pointer owner has to be " + ctx->highlightError("region"),
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
  return IR::PointerType::get(type->isVariable(), type->emit(ctx), getPointerOwner(ctx, ownerVal), isMultiPtr, ctx);
}

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

Json PointerType::toJson() const {
  return Json()
      ._("typeKind", "pointer")
      ._("isMulti", isMultiPtr)
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String PointerType::toString() const {
  return (String(isVariable() ? "var " : "") + (isMultiPtr ? "multiptr:[" : "ptr:[")) + type->toString() + "]";
}

} // namespace qat::ast
