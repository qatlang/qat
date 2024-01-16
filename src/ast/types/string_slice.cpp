#include "./string_slice.hpp"
#include "../../IR/types/string_slice.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

Maybe<usize> StringSliceType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
      {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx), ctx->dataLayout->getProgramAddressSpace()),
       llvm::Type::getInt64Ty(ctx->llctx)})));
}

IR::QatType* StringSliceType::emit(IR::Context* ctx) { return IR::StringSliceType::get(ctx); }

AstTypeKind StringSliceType::typeKind() const { return AstTypeKind::STRING_SLICE; }

Json StringSliceType::toJson() const { return Json()._("typeKind", "stringSlice")._("fileRange", fileRange); }

String StringSliceType::toString() const { return "str"; }

} // namespace qat::ast