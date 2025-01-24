#include "./string_slice.hpp"
#include "../../IR/types/string_slice.hpp"
#include "./type_kind.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

Maybe<usize> TextType::get_type_bitsize(EmitCtx* ctx) const {
	return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
	    llvm::StructType::create({llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                                                     ctx->irCtx->dataLayout->getProgramAddressSpace()),
	                              llvm::Type::getInt64Ty(ctx->irCtx->llctx)})));
}

ir::Type* TextType::emit(EmitCtx* ctx) { return ir::StringSliceType::get(ctx->irCtx); }

AstTypeKind TextType::type_kind() const { return AstTypeKind::TEXT; }

Json TextType::to_json() const { return Json()._("typeKind", "text")._("fileRange", fileRange); }

String TextType::to_string() const { return "text"; }

} // namespace qat::ast
