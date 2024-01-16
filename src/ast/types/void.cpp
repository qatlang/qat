#include "../../IR/types/void.hpp"
#include "./void.hpp"

namespace qat::ast {

Maybe<usize> VoidType::getTypeSizeInBits(IR::Context* ctx) const {
  return (
      usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::Type::getVoidTy(ctx->llctx)));
}

IR::QatType* VoidType::emit(IR::Context* ctx) { return IR::VoidType::get(ctx->llctx); }

AstTypeKind VoidType::typeKind() const { return AstTypeKind::VOID; }

Json VoidType::toJson() const { return Json()._("typeKind", "void")._("fileRange", fileRange); }

String VoidType::toString() const { return "void"; }

} // namespace qat::ast