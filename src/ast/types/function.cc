#include "./function.hpp"
#include "../../IR/types/function.hpp"

namespace qat::ast {

void FunctionType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                       EmitCtx* ctx) {
	returnType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	for (auto arg : argTypes) {
		arg->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
}

ir::Type* FunctionType::emit(EmitCtx* ctx) {
	Vec<ir::ArgumentType*> irArgTys;
	for (auto argTy : argTypes) {
		irArgTys.push_back(ir::ArgumentType::create_normal(argTy->emit(ctx), None, false));
	}
	if (isVariadic) {
		irArgTys.push_back(ir::ArgumentType::create_variadic(None));
	}
	ir::Type* retTy = returnType->emit(ctx);
	return new ir::FunctionType(ir::ReturnType::get(retTy), irArgTys, ctx->irCtx->llctx);
}

String FunctionType::to_string() const {
	String result("(");
	for (usize i = 0; i < argTypes.size(); i++) {
		result.append(argTypes[i]->to_string());
		if ((i != (argTypes.size() - 1)) || isVariadic) {
			result.append(", ");
		}
	}
	if (isVariadic) {
		result += "variadic";
	}
	result += ") -> " + returnType->to_string();
	return result;
}

Json FunctionType::to_json() const {
	Vec<JsonValue> args;
	for (auto argTy : argTypes) {
		args.push_back(argTy->to_json());
	}
	return Json()
	    ._("typeKind", "function")
	    ._("returnType", returnType->to_json())
	    ._("arguments", args)
	    ._("isVariadic", isVariadic);
}

} // namespace qat::ast
