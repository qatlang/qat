#include "./future.hpp"
#include "../../IR/types/future.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

namespace qat::ast {

void FutureType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                     EmitCtx* ctx) {
  subType->update_dependencies(phase, expect.value_or(ir::DependType::partial), ent, ctx);
}

Maybe<usize> FutureType::getTypeSizeInBits(EmitCtx* ctx) const {
  return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
      {llvm::Type::getInt64Ty(ctx->irCtx->llctx), llvm::Type::getInt64Ty(ctx->irCtx->llctx)->getPointerTo(),
       llvm::Type::getInt1Ty(ctx->irCtx->llctx)->getPointerTo(),
       llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo()})));
}

ir::Type* FutureType::emit(EmitCtx* ctx) { return ir::FutureType::get(subType->emit(ctx), isPacked, ctx->irCtx); }

AstTypeKind FutureType::type_kind() const { return AstTypeKind::FUTURE; }

Json FutureType::to_json() const {
  return Json()
      ._("typeKind", "future")
      ._("isPacked", isPacked)
      ._("subType", subType->to_json())
      ._("fileRange", fileRange);
}

String FutureType::to_string() const {
  return "future:[" + String(isPacked ? "pack, " : "") + subType->to_string() + "]";
}

} // namespace qat::ast
