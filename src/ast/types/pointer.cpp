#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

namespace qat::ast {

PointerType::PointerType(QatType* _type, bool _variable, PtrOwnType ownTy, Maybe<QatType*> _ownTyTy,
                         utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), type(_type), ownTyp(ownTy), ownerTyTy(_ownTyTy) {}

IR::PointerOwner PointerType::getPointerOwner(IR::Context* ctx) const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return IR::PointerOwner::OfType(ownerTyTy.value()->emit(ctx));
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
      return IR::PointerOwner::OfRegion();
  }
}

IR::QatType* PointerType::emit(IR::Context* ctx) {
  if (ownTyp == PtrOwnType::parent) {
    if (!ctx->fn && !ctx->activeType) {
      ctx->Error(
          "This pointer type is not inside a function or type and hence the pointer cannot have parent ownership",
          fileRange);
    }
  } else if (ownTyp == PtrOwnType::typeParent) {
    if (!ctx->activeType) {
      ctx->Error("This pointer type is not inside a type and hence the pointer cannot be owned by type", fileRange);
    }
  }
  return IR::PointerType::get(type->isVariable(), type->emit(ctx), getPointerOwner(ctx), ctx->llctx);
}

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

Json PointerType::toJson() const {
  return Json()
      ._("typeKind", "pointer")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String PointerType::toString() const { return (isVariable() ? "var #[" : "#[") + type->toString() + "]"; }

} // namespace qat::ast