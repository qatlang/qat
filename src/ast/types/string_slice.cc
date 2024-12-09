#include "./string_slice.hpp"
#include "../../IR/types/string_slice.hpp"
#include "./type_kind.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

Maybe<usize> StringSliceType::getTypeSizeInBits(EmitCtx* ctx) const {
	return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
	    llvm::StructType::create({llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                                                     ctx->irCtx->dataLayout->getProgramAddressSpace()),
	                              llvm::Type::getInt64Ty(ctx->irCtx->llctx)})));
}

ir::Type* StringSliceType::emit(EmitCtx* ctx) { return ir::StringSliceType::get(ctx->irCtx); }

AstTypeKind StringSliceType::type_kind() const { return AstTypeKind::STRING_SLICE; }

Json StringSliceType::to_json() const { return Json()._("typeKind", "stringSlice")._("fileRange", fileRange); }

String StringSliceType::to_string() const { return "str"; }

} // namespace qat::ast
