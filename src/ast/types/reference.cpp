#include "./reference.hpp"
#include "../../IR/types/reference.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

Maybe<usize> ReferenceType::getTypeSizeInBits(IR::Context* ctx) const {
  return (
      usize)(ctx->dataLayout->getTypeAllocSizeInBits(llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), 0u)));
}

IR::QatType* ReferenceType::emit(IR::Context* ctx) {
  auto* typEmit = type->emit(ctx);
  if (typEmit->isReference()) {
    ctx->Error("Subtype of reference cannot be a reference", fileRange);
  } else if (typEmit->isVoid()) {
    ctx->Error("Subtype of reference cannot be void", fileRange);
  }
  return IR::ReferenceType::get(isSubtypeVar, typEmit, ctx);
}

AstTypeKind ReferenceType::typeKind() const { return AstTypeKind::REFERENCE; }

Json ReferenceType::toJson() const {
  return Json()
      ._("typeKind", "reference")
      ._("subType", type->toJson())
      ._("isSubtypeVariable", isSubtypeVar)
      ._("fileRange", fileRange);
}

String ReferenceType::toString() const { return "@" + String(isSubtypeVar ? "var[" : "[") + type->toString() + "]"; }

} // namespace qat::ast