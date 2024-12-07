#include "./custom_float_literal.hpp"

#include <llvm/ADT/APFloat.h>

namespace qat::ast {

ir::PrerunValue* CustomFloatLiteral::emit(EmitCtx* ctx) {
	ir::Type* floatResTy = nullptr;
	if (is_type_inferred()) {
		if (inferredType->is_float() ||
			(inferredType->is_ctype() && inferredType->as_ctype()->get_subtype()->is_float())) {
			if (!kind.empty() && inferredType->to_string() != kind) {
				ctx->Error("The suffix provided here is " + ctx->color(kind) + " but the inferred type is " +
							   ctx->color(inferredType->to_string()),
						   fileRange);
			}
			floatResTy = inferredType;
		} else {
			ctx->Error("The type inferred from scope is " + ctx->color(inferredType->to_string()) +
						   " but this literal is expected to have a floating point type",
					   fileRange);
		}
	} else if (!kind.empty()) {
		if (kind == "f16") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_16, ctx->irCtx->llctx);
		} else if (kind == "fbrain") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_brain, ctx->irCtx->llctx);
		} else if (kind == "f32") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_32, ctx->irCtx->llctx);
		} else if (kind == "f64") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_64, ctx->irCtx->llctx);
		} else if (kind == "f80") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_80, ctx->irCtx->llctx);
		} else if (kind == "f128") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_128, ctx->irCtx->llctx);
		} else if (kind == "f128ppc") {
			floatResTy = ir::FloatType::get(ir::FloatTypeKind::_128PPC, ctx->irCtx->llctx);
		} else if (ir::native_type_kind_from_string(kind).has_value()) {
			floatResTy = ir::NativeType::get_from_kind(ir::native_type_kind_from_string(kind).value(), ctx->irCtx);
		} else {
			ctx->Error("Invalid suffix provided for custom float literal", fileRange);
		}
	} else {
		floatResTy = ir::FloatType::get(ir::FloatTypeKind::_64, ctx->irCtx->llctx);
	}
	return ir::PrerunValue::get(llvm::ConstantFP::get(floatResTy->get_llvm_type(), value), floatResTy);
}

String CustomFloatLiteral::to_string() const { return value + "_" + kind; }

Json CustomFloatLiteral::to_json() const {
	return Json()._("nodeType", "customFloatLiteral")._("nature", kind)._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast
