#include "./function_call.hpp"

namespace qat::ast {

ir::PrerunValue* PrerunFunctionCall::emit(EmitCtx* ctx) {
	auto candidate = funcExp->emit(ctx);
	if (not candidate->get_ir_type()->is_function()) {
		ctx->Error(
		    "This expression is not a prerun function. Expected the expression to have a function type, but instead it is of type " +
		        ctx->color(candidate->get_ir_type()->to_string()),
		    funcExp->fileRange);
	}
	auto       preFn    = reinterpret_cast<ir::PrerunFunction*>(candidate->get_llvm_constant());
	auto       fnTy     = preFn->get_ir_type()->as_function();
	const auto argCount = fnTy->get_argument_count();
	if (fnTy->is_variadic() && (argCount > arguments.size())) {
		ctx->Error("This prerun function is variadic and expects at least " +
		               ctx->color(std::to_string(fnTy->get_argument_count())) + " arguments to be provided",
		           fileRange);
	} else if ((not fnTy->is_variadic()) && (argCount != arguments.size())) {
		ctx->Error(
		    "The number of arguments does not match with the function's signature. The signature of the function is " +
		        ctx->color(fnTy->to_string()),
		    fileRange);
	}
	Vec<ir::PrerunValue*> argVals;
	for (usize i = 0; i < arguments.size(); i++) {
		auto* arg = arguments[i];
		if (arg->has_type_inferrance() && (i < argCount)) {
			arg->as_type_inferrable()->set_inference_type(fnTy->get_argument_type_at(i)->get_type());
		}
		auto argVal = arg->emit(ctx);
		if ((i < argCount) && not argVal->get_ir_type()->is_same(fnTy->get_argument_type_at(i)->get_type())) {
			ctx->Error("Expected an expression of type " + ctx->color(argVal->get_ir_type()->to_string()) +
			               " here, but found an expression of type " + ctx->color(argVal->get_ir_type()->to_string()) +
			               " instead. Please check your logic",
			           arg->fileRange);
		}
		argVals.push_back(argVal);
	}
	return preFn->call_prerun(argVals, ctx->irCtx, fileRange);
}

} // namespace qat::ast
