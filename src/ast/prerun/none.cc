#include "./none.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

ir::PrerunValue* NoneExpression::emit(EmitCtx* ctx) {
	if (type || is_type_inferred()) {
		if (is_type_inferred()) {
			if (!inferredType->is_maybe()) {
				ctx->Error("The inferred type of the " + ctx->color("none") + " expression is " +
				               ctx->color(inferredType->to_string()) + " which is not a maybe type",
				           fileRange);
			} else if (type && isPacked.has_value() && inferredType->is_maybe() &&
			           (!inferredType->as_maybe()->is_type_packed())) {
				ctx->Error("The inferred maybe type is " + ctx->color(inferredType->to_string()) +
				               " which is not packed, but " + ctx->color("pack") +
				               " is provided for the none expression. Please change this to " +
				               ctx->color("none:[" + type->to_string() + "]"),
				           isPacked.value());
			} else if (type && !isPacked.has_value() && inferredType->is_maybe() &&
			           inferredType->as_maybe()->is_type_packed()) {
				ctx->Error("The inferred maybe type is " + ctx->color(inferredType->to_string()) +
				               " which is packed, but " + ctx->color("pack") +
				               " is not provided in the none expression. Please change this to " +
				               ctx->color("none:[pack, " + type->to_string() + "]"),
				           fileRange);
			}
		}
		auto* typ = type ? type->emit(ctx) : inferredType->as_maybe()->get_subtype();
		if (inferredType && type) {
			if (!typ->is_same(inferredType->as_maybe()->get_subtype())) {
				ctx->Error("The type provided for this none expression is " + ctx->color(typ->to_string()) +
				               ", but the expected type is " + ctx->color(inferredType->as_maybe()->to_string()),
				           fileRange);
			}
		}
		SHOW("Type for none expression is: " << typ->to_string())
		auto* mTy = ir::MaybeType::get(typ, isPacked.has_value(), ctx->irCtx);
		return ir::PrerunValue::get(
		    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(mTy->get_llvm_type()),
		                              {llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u),
		                               llvm::Constant::getNullValue(mTy->get_subtype()->get_llvm_type())}),
		    mTy);
	} else {
		ctx->Error("No type found for the none expression. A type is required to be able to create the none value",
		           fileRange);
	}
	return nullptr;
}

String NoneExpression::to_string() const { return "none" + (type ? (":[" + type->to_string() + "]") : ""); }

Json NoneExpression::to_json() const {
	return Json()
	    ._("nodeType", "none")
	    ._("hasType", (type != nullptr))
	    ._("isPacked", isPacked.has_value())
	    ._("packRange", isPacked.has_value() ? isPacked.value() : JsonValue())
	    ._("type", (type != nullptr ? type->to_json() : Json()))
	    ._("fileRange", fileRange);
}

} // namespace qat::ast