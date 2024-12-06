#include "./maybe.hpp"
#include "../../IR/types/maybe.hpp"
#include "qat_type.hpp"

#include <llvm/IR/Module.h>

namespace qat::ast {

void MaybeType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                    EmitCtx* ctx) {
  subTyp->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

Maybe<usize> MaybeType::getTypeSizeInBits(EmitCtx* ctx) const {
  auto subTySize = subTyp->getTypeSizeInBits(ctx);
  if (subTySize.has_value()) {
    return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
        {llvm::Type::getInt1Ty(ctx->irCtx->llctx), llvm::Type::getIntNTy(ctx->irCtx->llctx, subTySize.value())})));
  } else {
    return None;
  }
}

ir::Type* MaybeType::emit(EmitCtx* ctx) {
  SHOW("Emitting subtype")
  auto* subType = subTyp->emit(ctx);
  SHOW("Subtype emitted")
  if (subType->is_opaque() && !subType->as_opaque()->has_subtype()) {
    ctx->Error("The subtype of " + ctx->color("maybe") +
                   " is an incomplete opaque type: " + ctx->color(subType->to_string()),
               fileRange);
  }
  return ir::MaybeType::get(subType, isPacked, ctx->irCtx);
}

Json MaybeType::to_json() const {
  return Json()
      ._("typeKind", "maybe")
      ._("isPacked", isPacked)
      ._("subType", subTyp->to_json())
      ._("fileRange", fileRange);
}

String MaybeType::to_string() const { return String(isPacked ? "pack " : "") + "maybe:[" + subTyp->to_string() + "]"; }

} // namespace qat::ast
