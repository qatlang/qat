#include "./reference.hpp"
#include "../../IR/types/reference.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

ReferenceType::ReferenceType(QatType* _type, bool _variable, FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), type(_type) {}

Maybe<usize> ReferenceType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), 0u)));
}

IR::QatType* ReferenceType::emit(IR::Context* ctx) {
  auto* typEmit = type->emit(ctx);
  if (typEmit->isReference()) {
    ctx->Error("Subtype of reference cannot be a reference", fileRange);
  } else if (typEmit->isVoid()) {
    ctx->Error("Subtype of reference cannot be void", fileRange);
  }
  return IR::ReferenceType::get(type->isVariable(), typEmit, ctx);
}

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

Json ReferenceType::toJson() const {
  return Json()
      ._("typeKind", "reference")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String ReferenceType::toString() const { return (isVariable() ? "var @" : "@") + type->toString(); }

} // namespace qat::ast