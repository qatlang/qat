#include "./give_sentence.hpp"
#include "../../IR/types/void.hpp"
#include "./internal_exceptions.hpp"

namespace qat::ast {

void PrerunGive::emit(EmitCtx* ctx) {
	if (ctx->has_pre_call_state()) {
		if (value.has_value()) {
			if (value.value()->has_type_inferrance()) {
				value.value()->as_type_inferrable()->set_inference_type(
				    ctx->get_pre_call_state()->get_function()->get_return_type());
			}
			auto retVal = value.value()->emit(ctx);
			if (ctx->get_pre_call_state()->get_function()->get_return_type()->is_same(retVal->get_ir_type())) {
				throw InternalPrerunGive{retVal};
			} else {
				ctx->Error("This function expects a given value of type " +
				               ctx->color(ctx->get_pre_call_state()->get_function()->get_return_type()->to_string()) +
				               " but the provided expression is of type " +
				               ctx->color(retVal->get_ir_type()->to_string()),
				           fileRange);
			}
		} else {
			if (not ctx->get_pre_call_state()->get_function()->get_return_type()->is_void()) {
				ctx->Error("The given type of this function is " +
				               ctx->color(ctx->get_pre_call_state()->get_function()->get_return_type()->to_string()) +
				               ", so a value should be provided",
				           fileRange);
			}
			throw InternalPrerunGive{ir::PrerunValue::get(nullptr, ir::VoidType::get(ctx->irCtx->llctx))};
		}
	} else {
		ctx->Error("No function call state for this give sentence", fileRange);
	}
}

} // namespace qat::ast
