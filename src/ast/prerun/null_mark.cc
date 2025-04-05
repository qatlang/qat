#include "./null_mark.hpp"
#include "../../IR/logic.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::PrerunValue* NullPointer::emit(EmitCtx* ctx) {
	if (not providedType.has_value() && !is_type_inferred()) {
		ctx->Error("No type provided for null pointer and no type inferred from scope", fileRange);
	}
	auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
	if (providedType.has_value() && is_type_inferred() && not theType->is_same(inferredType)) {
		ctx->Error("The provided type for the null pointer is " + ctx->color(theType->to_string()) +
		               ", but the inferred type is " + ctx->color(inferredType->to_string()),
		           fileRange);
	}
	ir::Type* finalTy = theType;
	if (theType->is_ptr()) {
		if (theType->as_ptr()->is_non_nullable()) {
			ctx->Error("The inferred type is " + ctx->color(theType->to_string()) +
			               " which is not a nullable pointer type",
			           fileRange);
		}
		if (theType->is_native_type()) {
			finalTy = theType->as_native_type()->get_subtype();
		}
	} else {
		ctx->Error("The inferred type for null is " + ctx->color(theType->to_string()) + " which is not a pointer type",
		           fileRange);
	}
	SHOW("Null pointer inferred type: " << finalTy->to_string())
	auto llPtrTy =
	    llvm::PointerType::get(llvm::PointerType::isValidElementType(finalTy->as_ptr()->get_subtype()->get_llvm_type())
	                               ? finalTy->as_ptr()->get_subtype()->get_llvm_type()
	                               : llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                           ctx->irCtx->dataLayout.getProgramAddressSpace());
	;
	return ir::PrerunValue::get(
	    finalTy->as_ptr()->is_multi()
	        ? llvm::cast<llvm::Constant>(llvm::ConstantAggregateZero::get(finalTy->get_llvm_type()))
	        : llvm::ConstantPointerNull::get(llPtrTy),
	    theType);
}

String NullPointer::to_string() const {
	return "null" + (providedType.has_value() ? (":[" + providedType.value()->to_string() + "]") : "");
}

Json NullPointer::to_json() const { return Json()._("nodeType", "nullPointer")._("fileRange", fileRange); }

} // namespace qat::ast
