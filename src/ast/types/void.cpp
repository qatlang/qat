#include "../../IR/types/void.hpp"
#include "./void.hpp"

namespace qat::ast {

VoidType::VoidType(bool _variable, FileRange _fileRange) : QatType(_variable, std::move(_fileRange)) {}

Maybe<usize> VoidType::getTypeSizeInBits(IR::Context* ctx) const {
  return (
      usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::Type::getVoidTy(ctx->llctx)));
}

IR::QatType* VoidType::emit(IR::Context* ctx) { return IR::VoidType::get(ctx->llctx); }

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

Json VoidType::toJson() const {
  return Json()._("typeKind", "void")._("isVariable", isVariable())._("fileRange", fileRange);
}

String VoidType::toString() const { return isVariable() ? "var void" : "void"; }

} // namespace qat::ast