#include "./array_literal.hpp"
#include "../../IR/types/array.hpp"
#include "../types/qat_type.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

void PrerunArrayLiteral::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent,
                                             EmitCtx* ctx) {
	if (elemTyHint) {
		UPDATE_DEPS(elemTyHint);
	}
	for (auto val : valuesExp) {
		UPDATE_DEPS(val);
	}
}

ir::PrerunValue* PrerunArrayLiteral::emit(EmitCtx* ctx) {
	ir::Type* elementType = nullptr;
	if (elemTyHint) {
		elementType = elemTyHint->emit(ctx);
	}
	ir::ArrayType* arrTy = nullptr;
	if (is_type_inferred()) {
		if (not inferredType->is_array()) {
			ctx->Error("This expression expects an array type, but the type inferred from scope is " +
			               ctx->color(inferredType->to_string()),
			           fileRange);
		}
		arrTy = inferredType->as_array();
		if (inferredType->as_array()->get_length() != valuesExp.size()) {
			ctx->Error("The inferred type is " + ctx->color(inferredType->to_string()) + " expecting " +
			               ctx->color(std::to_string(inferredType->as_array()->get_length())) + " elements, but " +
			               ctx->color(std::to_string(valuesExp.size())) + " values were provided instead",
			           fileRange);
		}
		if (elementType) {
			if (not elementType->is_same(inferredType->as_array()->get_element_type())) {
				ctx->Error(
				    "The hint provided about the type of the element of this array literal is " +
				        ctx->color(elementType->to_string()) +
				        ", which does not match with the element type of the array type as inferred from scope, which is " +
				        ctx->color(inferredType->to_string()),
				    fileRange);
			}
		} else {
			elementType = arrTy->get_element_type();
		}
	}
	Vec<llvm::Constant*> constVals;
	for (usize i = 0; i < valuesExp.size(); i++) {
		if (elementType && valuesExp[i]->has_type_inferrance()) {
			valuesExp[i]->as_type_inferrable()->set_inference_type(elementType);
		}
		auto itVal = valuesExp.at(i)->emit(ctx);
		if (elementType) {
			if (not elementType->is_same(itVal->get_ir_type())) {
				ctx->Error(
				    "Type of this expression is " + ctx->color(itVal->get_ir_type()->to_string()) +
				        " which does not match the type of the element of the array as inferred from scope, which is " +
				        ctx->color(elementType->to_string()),
				    valuesExp[i]->fileRange);
			}
		} else {
			elementType = itVal->get_ir_type();
		}
		constVals.push_back(itVal->get_llvm_constant());
	}
	if (not elementType) {
		ctx->Error("This is an empty array literal. Either the type of this expression should be inferred from scope,"
		           " or a hint about the element of the array should be provided. You can use the syntax " +
		               ctx->color("[]:[ElementType]") + " to provide hint about the element type of the array",
		           fileRange);
	}
	if (not arrTy) {
		arrTy = ir::ArrayType::get(elementType, valuesExp.size(), ctx->irCtx->llctx);
	}
	return ir::PrerunValue::get(
	    llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTy->get_llvm_type()), constVals), arrTy);
}

String PrerunArrayLiteral::to_string() const {
	String result("[");
	for (usize i = 0; i < valuesExp.size(); i++) {
		result += valuesExp[i]->to_string();
		if (i != (valuesExp.size() - 1)) {
			result += ", ";
		}
	}
	result += "]";
	return result;
}

} // namespace qat::ast
