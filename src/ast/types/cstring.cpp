#include "../../IR/types/cstring.hpp"
#include "./cstring.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

CStringType::CStringType(bool _variable, FileRange _fileRange) : QatType(_variable, std::move(_fileRange)) {}

Maybe<usize> CStringType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), 0u)));
}

IR::QatType* CStringType::emit(IR::Context* ctx) { return IR::CStringType::get(ctx->llctx); }

TypeKind CStringType::typeKind() const { return TypeKind::cstring; }

Json CStringType::toJson() const {
  return Json()._("typeKind", "cstring")._("isVariable", isVariable())._("fileRange", fileRange);
}

String CStringType::toString() const { return isVariable() ? "var cstring" : "cstring"; }

} // namespace qat::ast