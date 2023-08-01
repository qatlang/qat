#include "./string_slice.hpp"
#include "../../IR/types/string_slice.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

StringSliceType::StringSliceType(bool _variable, FileRange _fileRange) : QatType(_variable, std::move(_fileRange)) {}

Maybe<usize> StringSliceType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
      {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()),
       llvm::Type::getInt64Ty(ctx->llctx)})));
}

IR::QatType* StringSliceType::emit(IR::Context* ctx) { return IR::StringSliceType::get(ctx->llctx); }

TypeKind StringSliceType::typeKind() const { return TypeKind::stringSlice; }

Json StringSliceType::toJson() const {
  return Json()._("typeKind", "stringSlice")._("isVariable", isVariable())._("fileRange", fileRange);
}

String StringSliceType::toString() const { return isVariable() ? "var str" : "str"; }

} // namespace qat::ast