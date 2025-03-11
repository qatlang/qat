#include "./give_sentence.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* GiveSentence::emit(EmitCtx* ctx) {
	auto* fun = ctx->get_fn();
	if (give_expr.has_value()) {
		if (fun->get_ir_type()->as_function()->get_return_type()->is_return_self()) {
			if (give_expr.value()->nodeType() != NodeType::SELF) {
				ctx->Error("This function is marked to return '' and cannot return anything else", fileRange);
			}
		}
		auto* retType = fun->get_ir_type()->as_function()->get_return_type()->get_type();
		if (give_expr.value()->has_type_inferrance()) {
			give_expr.value()->as_type_inferrable()->set_inference_type(retType);
		}
		auto* retVal = give_expr.value()->emit(ctx);
		SHOW("RetType: " << retType->to_string() << "\nRetValType: " << retVal->get_ir_type()->to_string())
		if (retType->is_same(retVal->get_ir_type())) {
			if (retType->is_void()) {
				ir::destructor_caller(ctx->irCtx, fun);
				ir::method_handler(ctx->irCtx, fun);
				return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), retType, false);
			} else {
				if (retVal->is_ghost_ref()) {
					if (retType->has_simple_copy() || retType->has_simple_move()) {
						auto* loadRes = ctx->irCtx->builder.CreateLoad(retType->get_llvm_type(), retVal->get_llvm());
						if (not retType->has_simple_copy()) {
							if (not retVal->is_variable()) {
								ctx->Error(
								    "This expression does not have variability, and hence simple-move is not possible",
								    give_expr.value()->fileRange);
							}
							ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(retType->get_llvm_type()),
							                                retVal->get_llvm());
						}
						ir::destructor_caller(ctx->irCtx, fun);
						ir::method_handler(ctx->irCtx, fun);
						return ir::Value::get(ctx->irCtx->builder.CreateRet(loadRes), retType, false);
					} else {
						ctx->Error(
						    "This expression is of type " + ctx->color(retType->to_string()) +
						        " which does not have simple-copy and simple-move. To convert the expression to a value, please use " +
						        ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
						    fileRange);
					}
				} else {
					ir::destructor_caller(ctx->irCtx, fun);
					ir::method_handler(ctx->irCtx, fun);
					return ir::Value::get(ctx->irCtx->builder.CreateRet(retVal->get_llvm()), retType, false);
				}
			}
		} else if (retType->is_ref() &&
		           retType->as_ref()->get_subtype()->is_same(
		               retVal->is_ref() ? retVal->get_ir_type()->as_ref()->get_subtype() : retVal->get_ir_type()) &&
		           (retType->as_ref()->has_variability()
		                ? (retVal->is_ghost_ref()
		                       ? retVal->is_variable()
		                       : (retVal->is_ref() && retVal->get_ir_type()->as_ref()->has_variability()))
		                : true)) {
			SHOW("Return type is compatible")
			if (retType->is_ref() && not retVal->is_ref() && retVal->is_local_value()) {
				ctx->irCtx->Warning(
				    "Returning reference to a local value. The value pointed to by the reference might be destroyed before the function call is complete",
				    fileRange);
			}
			if (retVal->is_ref()) {
				retVal->load_ghost_ref(ctx->irCtx->builder);
			}
			ir::destructor_caller(ctx->irCtx, fun);
			ir::method_handler(ctx->irCtx, fun);
			return ir::Value::get(ctx->irCtx->builder.CreateRet(retVal->get_llvm()), retType, false);
		} else if (retVal->get_ir_type()->is_ref() &&
		           retVal->get_ir_type()->as_ref()->get_subtype()->is_same(retType)) {
			if (retType->has_simple_copy() || retType->has_simple_move()) {
				retVal->load_ghost_ref(ctx->irCtx->builder);
				auto* loadRes = ctx->irCtx->builder.CreateLoad(retType->get_llvm_type(), retVal->get_llvm());
				if (not retType->has_simple_copy()) {
					if (not retVal->get_ir_type()->as_ref()->has_variability()) {
						ctx->Error(
						    "The expression is of type " + ctx->irCtx->color(retVal->get_ir_type()->to_string()) +
						        " which is a reference without variability, and hence simple-move is not possible",
						    give_expr.value()->fileRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(retType->get_llvm_type()),
					                                retVal->get_llvm());
				}
				ir::destructor_caller(ctx->irCtx, fun);
				ir::method_handler(ctx->irCtx, fun);
				return ir::Value::get(ctx->irCtx->builder.CreateRet(loadRes), retType, false);
			} else {
				ctx->Error(
				    "This expression is of type " + ctx->color(retType->to_string()) +
				        " which does not have simple-copy and simple-move. To convert the expression to a value, please use " +
				        ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
				    fileRange);
			}
		} else {
			ctx->Error("Given value type of the function is " +
			               fun->get_ir_type()->as_function()->get_return_type()->to_string() +
			               ", but the provided value in the give sentence is " + retVal->get_ir_type()->to_string(),
			           give_expr.value()->fileRange);
		}
	} else {
		if (fun->get_ir_type()->as_function()->get_return_type()->get_type()->is_void()) {
			ir::destructor_caller(ctx->irCtx, fun);
			ir::method_handler(ctx->irCtx, fun);
			return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), ir::VoidType::get(ctx->irCtx->llctx), false);
		} else {
			ctx->Error("No value is provided to give while the given type of the function is " +
			               ctx->color(fun->get_ir_type()->as_function()->get_return_type()->to_string()) +
			               ". Please provide a value of the appropriate type",
			           fileRange);
		}
	}
}

Json GiveSentence::to_json() const {
	return Json()
	    ._("nodeType", "giveSentence")
	    ._("hasValue", give_expr.has_value())
	    ._("value", give_expr.has_value() ? give_expr.value()->to_json() : Json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
