#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

Maybe<usize> UnsignedType::getTypeSizeInBits(EmitCtx* ctx) const {
	return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
	    llvm::Type::getIntNTy(ctx->irCtx->llctx, bitWidth)));
}

ir::Type* UnsignedType::emit(EmitCtx* ctx) {
	if (bitWidth > 128) {
		ctx->Error("Arbitrary unsigned integer bitwidths above 128 are not allowed at the moment", fileRange);
	}
	if (is_bool) {
		return ir::UnsignedType::create_bool(ctx->irCtx);
	} else if (isPartOfGeneric || ctx->mod->has_unsigned_bitwidth(bitWidth)) {
		return ir::UnsignedType::create(bitWidth, ctx->irCtx);
	} else {
		ctx->Error("The unsigned integer bitwidth " + ctx->color(std::to_string(bitWidth)) +
		               " is not allowed to be used since it is not brought into the module " +
		               ctx->color(ctx->mod->get_name()) + " in file " + ctx->mod->get_file_path(),
		           fileRange);
	}
	return nullptr;
}

bool UnsignedType::isBitWidth(const u32 width) const { return bitWidth == width; }

AstTypeKind UnsignedType::type_kind() const { return AstTypeKind::UNSIGNED_INTEGER; }

Json UnsignedType::to_json() const {
	return Json()
	    ._("typeKind", "unsignedInteger")
	    ._("is_bool", is_bool)
	    ._("bitWidth", bitWidth)
	    ._("fileRange", fileRange);
}

String UnsignedType::to_string() const { return is_bool ? "bool" : ("u" + std::to_string(bitWidth)); }

} // namespace qat::ast
