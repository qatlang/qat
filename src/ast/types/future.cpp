#include "./future.hpp"
#include "../../IR/types/future.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

Maybe<usize> FutureType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
      {llvm::Type::getInt64Ty(ctx->llctx), llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
       llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(), llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()})));
}

IR::QatType* FutureType::emit(IR::Context* ctx) { return IR::FutureType::get(subType->emit(ctx), isPacked, ctx); }

AstTypeKind FutureType::typeKind() const { return AstTypeKind::FUTURE; }

Json FutureType::toJson() const {
  return Json()
      ._("typeKind", "future")
      ._("isPacked", isPacked)
      ._("subType", subType->toJson())
      ._("fileRange", fileRange);
}

String FutureType::toString() const {
  return "future:[" + String(isPacked ? "pack, " : "") + subType->toString() + "]";
}

} // namespace qat::ast