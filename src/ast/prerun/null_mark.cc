#include "./null_mark.hpp"
#include "../../IR/logic.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::PrerunValue* NullMark::emit(EmitCtx* ctx) {
	if (not providedType.has_value() && !is_type_inferred()) {
		ctx->Error("No type provided for null mark and no type inferred from scope", fileRange);
	}
	auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
	if (providedType.has_value() && is_type_inferred()) {
		if (not theType->is_same(inferredType)) {
			ctx->Error("The provided type for the null mark is " + ctx->color(theType->to_string()) +
			               ", but the inferred type is " + ctx->color(inferredType->to_string()),
			           fileRange);
		}
	}
	ir::Type* finalTy = theType;
	if (theType->is_mark() || (theType->is_native_type() && theType->as_native_type()->get_subtype()->is_mark())) {
		if (theType->is_native_type() ? theType->as_native_type()->get_subtype()->as_mark()->is_non_nullable()
		                              : theType->as_mark()->is_non_nullable()) {
			ctx->Error("The inferred type is " + ctx->color(theType->to_string()) +
			               " which is not a nullable mark type",
			           fileRange);
		}
		if (theType->is_native_type()) {
			finalTy = theType->as_native_type()->get_subtype();
		}
	} else {
		ctx->Error("The inferred type for null is " + ctx->color(theType->to_string()) + " which is not a mark type",
		           fileRange);
	}
	auto llPtrTy =
	    llvm::PointerType::get(llvm::PointerType::isValidElementType(finalTy->as_mark()->get_subtype()->get_llvm_type())
	                               ? finalTy->as_mark()->get_subtype()->get_llvm_type()
	                               : llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                           ctx->irCtx->dataLayout.getProgramAddressSpace());
	return ir::PrerunValue::get(
	    finalTy->as_mark()->is_slice()
	        ? llvm::ConstantStruct::get(llvm::dyn_cast<llvm::StructType>(finalTy->get_llvm_type()),
	                                    {llvm::ConstantPointerNull::get(llPtrTy),
	                                     llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)})
	        : llvm::ConstantPointerNull::get(llPtrTy),
	    theType);
}

String NullMark::to_string() const {
	return "null" + (providedType.has_value() ? (":[" + providedType.value()->to_string() + "]") : "");
}

Json NullMark::to_json() const { return Json()._("nodeType", "nullMark")._("fileRange", fileRange); }

} // namespace qat::ast
