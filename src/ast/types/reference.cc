#include "./reference.hpp"
#include "../../IR/types/reference.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

void ReferenceType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                        EmitCtx* ctx) {
  type->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

Maybe<usize> ReferenceType::getTypeSizeInBits(EmitCtx* ctx) const {
  return (usize)(ctx->irCtx->dataLayout->getTypeAllocSizeInBits(
      llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u)));
}

ir::Type* ReferenceType::emit(EmitCtx* ctx) {
  auto* typEmit = type->emit(ctx);
  if (typEmit->is_reference() || typEmit->is_void() || typEmit->is_region()) {
    ctx->Error("Subtype of reference cannot be " + ctx->color(typEmit->to_string()), fileRange);
  }
  return ir::ReferenceType::get(isSubtypeVar, typEmit, ctx->irCtx);
}

AstTypeKind ReferenceType::type_kind() const { return AstTypeKind::REFERENCE; }

Json ReferenceType::to_json() const {
  return Json()
      ._("typeKind", "reference")
      ._("subType", type->to_json())
      ._("isSubtypeVariable", isSubtypeVar)
      ._("fileRange", fileRange);
}

String ReferenceType::to_string() const { return "@" + String(isSubtypeVar ? "var[" : "[") + type->to_string() + "]"; }

} // namespace qat::ast