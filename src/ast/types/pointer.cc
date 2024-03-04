#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

void PointerType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                      EmitCtx* ctx) {
  type->update_dependencies(phase, ir::DependType::partial, ent, ctx);
  if (ownerTyTy.has_value()) {
    ownerTyTy.value()->update_dependencies(phase, ir::DependType::partial, ent, ctx);
  }
}

Maybe<usize> PointerType::getTypeSizeInBits(EmitCtx* ctx) const {
  return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
      isMultiPtr ? llvm::cast<llvm::Type>(llvm::StructType::create(
                       {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
                                               ctx->irCtx->dataLayout->getProgramAddressSpace()),
                        llvm::Type::getInt64Ty(ctx->irCtx->llctx)}))
                 : llvm::cast<llvm::Type>(llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
                                                                 ctx->irCtx->dataLayout->getProgramAddressSpace()))));
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

ir::PointerOwner PointerType::getPointerOwner(EmitCtx* ctx, Maybe<ir::Type*> ownerVal) const {
  switch (ownTyp) {
    case PtrOwnType::type:
      return ir::PointerOwner::OfType(ownerVal.value());
    case PtrOwnType::typeParent: {
      if (ctx->has_member_parent()) {
        return ir::PointerOwner::OfParentInstance(ctx->get_member_parent()->get_parent_type());
      } else {
        ctx->Error("No parent type or skill found", fileRange);
      }
    }
    case PtrOwnType::anonymous:
      return ir::PointerOwner::OfAnonymous();
    case PtrOwnType::heap:
      return ir::PointerOwner::OfHeap();
    case PtrOwnType::function:
      return ir::PointerOwner::OfParentFunction(ctx->get_fn());
    case PtrOwnType::region:
      return ir::PointerOwner::OfRegion(ownerVal.value()->as_region());
    case PtrOwnType::anyRegion:
      return ir::PointerOwner::OfAnyRegion();
  }
}

ir::Type* PointerType::emit(EmitCtx* ctx) {
  if (ownTyp == PtrOwnType::function) {
    if (!ctx->get_fn()) {
      ctx->Error("This pointer type is not inside a function and hence cannot have function ownership", fileRange);
    }
  } else if (ownTyp == PtrOwnType::typeParent) {
    if (!(ctx->has_member_parent())) {
      ctx->Error("No parent type found in scope and hence the pointer "
                 "cannot be owned by the parent type instance",
                 fileRange);
    }
  }
  Maybe<ir::Type*> ownerVal;
  if (ownTyp == PtrOwnType::type) {
    if (!ownerTyTy) {
      ctx->Error("Expected a type to be provided for pointer ownership", fileRange);
    }
    auto* typVal = ownerTyTy.value()->emit(ctx);
    if (typVal->is_region()) {
      ctx->Error("The type provided is a region and hence pointer ownership has to be " +
                     ctx->color("'region(" + typVal->to_string() = ")") + " or " + ctx->color("'region"),
                 fileRange);
    }
    ownerVal = typVal;
  } else if (ownTyp == PtrOwnType::region) {
    if (ownerTyTy) {
      auto* regTy = ownerTyTy.value()->emit(ctx);
      if (!regTy->is_region()) {
        ctx->Error("The provided type is not a region type and hence pointer "
                   "owner cannot be " +
                       ctx->color("region"),
                   fileRange);
      }
      ownerVal = regTy;
    }
  }
  auto subTy = type->emit(ctx);
  if (isSubtypeVar && subTy->is_function()) {
    ctx->Error(
        "The subtype of this pointer type is a function type, and hence variability cannot be specified here. Please remove " +
            ctx->color("var"),
        fileRange);
  }
  return ir::PointerType::get(isSubtypeVar, subTy, isNonNullable, getPointerOwner(ctx, ownerVal), isMultiPtr,
                              ctx->irCtx);
}

AstTypeKind PointerType::type_kind() const { return AstTypeKind::POINTER; }

Json PointerType::to_json() const {
  return Json()
      ._("type_kind", "pointer")
      ._("isMulti", isMultiPtr)
      ._("isSubtypeVariable", isSubtypeVar)
      ._("subType", type->to_json())
      ._("ownerKind", pointerOwnerToString())
      ._("hasOwnerType", ownerTyTy.has_value())
      ._("ownerType", ownerTyTy.has_value() ? ownerTyTy.value()->to_json() : Json())
      ._("fileRange", fileRange);
}

String PointerType::to_string() const {
  Maybe<String> ownerStr;
  switch (ownTyp) {
    case PtrOwnType::type: {
      ownerStr = String("type(") + ownerTyTy.value()->to_string() + ")";
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
      ownerStr = String("region(") + ownerTyTy.value()->to_string() + ")";
      break;
    }
    case PtrOwnType::anyRegion: {
      ownerStr = "region";
      break;
    }
    default:;
  }
  return (isMultiPtr ? "multiptr:[" : "ptr:[") + String(isSubtypeVar ? "var " : "") + type->to_string() +
         (ownerStr.has_value() ? (", " + ownerStr.value()) : "") + "]";
}

} // namespace qat::ast
