#include "./give_sentence.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* GiveSentence::emit(EmitCtx* ctx) {
	auto* fun = ctx->get_fn();
	if (value) {
		if (fun->get_ir_type()->as_function()->get_return_type()->is_return_self()) {
			if (value->nodeType() != NodeType::SELF) {
				ctx->Error("This function is defined to return '' and cannot return anything else", fileRange);
			}
			auto expr = value->emit(ctx);
			return ir::Value::get(ctx->irCtx->builder.CreateRet(expr->get_llvm()), expr->get_ir_type(), false);
		}
		auto* retType = fun->get_ir_type()->as_function()->get_return_type()->get_type();
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(retType);
		}
		auto* retVal = value->emit(ctx);
		if (retType->is_void() && retVal->get_ir_type()->is_void()) {
			ir::destructor_caller(ctx->irCtx, fun);
			ir::method_handler(ctx->irCtx, fun);
			return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), retType, false);
		} else {
			if (retType->is_void() || retVal->get_ir_type()->is_void()) {
				ctx->Error("Given type of this function is " + ctx->color(retType->to_string()) +
				               ", but the provided expression is of type " +
				               ctx->color(retVal->get_ir_type()->to_string()),
				           fileRange);
			}
			auto retRes = ir::Logic::handle_pass_semantics(ctx, retType, retVal, value->fileRange, true);
			ir::destructor_caller(ctx->irCtx, fun);
			ir::method_handler(ctx->irCtx, fun);
			return ir::Value::get(ctx->irCtx->builder.CreateRet(retRes->get_llvm()), retType, false);
		}
	} else {
		if (not fun->get_ir_type()->as_function()->get_return_type()->get_type()->is_void()) {
			ctx->Error("No value is provided here while the given type of the function is " +
			               ctx->color(fun->get_ir_type()->as_function()->get_return_type()->to_string()) +
			               ". Please provide a value of the appropriate type",
			           fileRange);
		}
		ir::destructor_caller(ctx->irCtx, fun);
		ir::method_handler(ctx->irCtx, fun);
		return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), ir::VoidType::get(ctx->irCtx->llctx), false);
	}
}

Json GiveSentence::to_json() const {
	return Json()
	    ._("nodeType", "giveSentence")
	    ._("hasValue", value != nullptr)
	    ._("value", value ? value->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
