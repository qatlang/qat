#include "./negative.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

ir::PrerunValue* PrerunNegative::emit(EmitCtx* ctx) {
	if (is_type_inferred()) {
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(inferredType);
		}
	}
	auto* irVal = value->emit(ctx);
	if (is_type_inferred()) {
		if (!irVal->get_ir_type()->is_same(inferredType)) {
			ctx->Error("The expression is of type " + ctx->color(irVal->get_ir_type()->to_string()) +
						   ", but the type inferred from scope is " + ctx->color(inferredType->to_string()),
					   value->fileRange);
		}
	}
	if (irVal->get_ir_type()->is_integer() || (irVal->get_ir_type()->is_native_type() &&
											   irVal->get_ir_type()->as_native_type()->get_subtype()->is_integer())) {
		return ir::PrerunValue::get(llvm::ConstantFoldConstant(llvm::ConstantExpr::getNeg(irVal->get_llvm_constant()),
															   ctx->irCtx->dataLayout.value()),
									irVal->get_ir_type());
	} else if (irVal->get_ir_type()->is_float() ||
			   (irVal->get_ir_type()->is_native_type() &&
				irVal->get_ir_type()->as_native_type()->get_subtype()->is_float())) {
		return ir::PrerunValue::get(
			llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFNeg(irVal->get_llvm_constant())),
			irVal->get_ir_type());
	} else if (irVal->get_ir_type()->is_unsigned_integer() ||
			   (irVal->get_ir_type()->is_native_type() &&
				irVal->get_ir_type()->as_native_type()->get_subtype()->is_integer())) {
		ctx->Error("Expression of unsigned integer type " + ctx->color(irVal->get_ir_type()->to_string()) +
					   " cannot use the " + ctx->color("unary -") + " operator",
				   fileRange);
	} else {
		// FIXME - Support prerun operator functions
		ctx->Error("Type " + ctx->color(irVal->get_ir_type()->to_string()) + " does not support the " +
					   ctx->color("unary -") + " operator",
				   fileRange);
	}
	return nullptr;
}

String PrerunNegative::to_string() const { return "-" + value->to_string(); }

Json PrerunNegative::to_json() const {
	return Json()._("nodeType", "prerunNegative")._("subExpression", value->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
