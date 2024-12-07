#include "./array_literal.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::PrerunValue* PrerunArrayLiteral::emit(EmitCtx* ctx) {
	if (is_type_inferred()) {
		if (!inferredType->is_array()) {
			ctx->Error("This expression expects an array type, but the type inferred from scope is " +
						   ctx->color(inferredType->to_string()),
					   fileRange);
		}
		if (inferredType->as_array()->get_length() != valuesExp.size()) {
			ctx->Error("The inferred type is " + ctx->color(inferredType->to_string()) + " expecting " +
						   ctx->color(std::to_string(inferredType->as_array()->get_length())) + " elements, but " +
						   ctx->color(std::to_string(valuesExp.size())) + " values were provided instead",
					   fileRange);
		}
	}
	ir::Type*			 elementType = nullptr;
	Vec<llvm::Constant*> constVals;
	for (usize i = 0; i < valuesExp.size(); i++) {
		if (is_type_inferred() && valuesExp[i]->has_type_inferrance()) {
			valuesExp[i]->as_type_inferrable()->set_inference_type(inferredType->as_array()->get_element_type());
		}
		auto itVal = valuesExp.at(i)->emit(ctx);
		if (is_type_inferred()) {
			if (!inferredType->as_array()->get_element_type()->is_same(itVal->get_ir_type())) {
				ctx->Error("This expression is of type " + ctx->color(itVal->get_ir_type()->to_string()) +
							   " which does not match with the expected element type of the inferred type, which is " +
							   ctx->color(inferredType->as_array()->get_element_type()->to_string()),
						   valuesExp[i]->fileRange);
			}
		} else {
			if (elementType) {
				if (!elementType->is_same(itVal->get_ir_type())) {
					ctx->Error("Type of this expression is " + ctx->color(itVal->get_ir_type()->to_string()) +
								   " which does not match the type of the previous elements, which is " +
								   ctx->color(elementType->to_string()),
							   valuesExp[i]->fileRange);
				}
			} else {
				elementType = itVal->get_ir_type();
			}
		}
		constVals.push_back(itVal->get_llvm_constant());
	}
	return ir::PrerunValue::get(
		llvm::ConstantArray::get(is_type_inferred()
									 ? llvm::cast<llvm::ArrayType>(inferredType->get_llvm_type())
									 : llvm::ArrayType::get(elementType->get_llvm_type(), constVals.size()),
								 constVals),
		is_type_inferred() ? inferredType : ir::ArrayType::get(elementType, constVals.size(), ctx->irCtx->llctx));
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

Json PrerunArrayLiteral::to_json() const {
	Vec<JsonValue> valuesJson;
	return Json()._("nodeType", "arrayLiteral")._("values", valuesJson)._("fileRange", fileRange);
}

} // namespace qat::ast
