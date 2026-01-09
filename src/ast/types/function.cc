#include "./function.hpp"
#include "../../IR/types/function.hpp"

namespace qat::ast {

void FunctionType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	returnType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	for (auto arg : argTypes) {
		arg->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
	if (variadics.has_value() && variadics.value().kind == VariadicKind::TYPED) {
		variadics.value().type->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
}

ir::Type* FunctionType::emit(EmitCtx* ctx) {
	Vec<ir::ArgumentType*> irArgTys;
	for (auto argTy : argTypes) {
		irArgTys.push_back(ir::ArgumentType::create_normal(argTy->emit(ctx), None, false));
	}
	Maybe<ir::Variadics> variadicsIR;
	if (variadics.has_value()) {
		auto      kind = ir::VariadicsKind::NORMAL;
		ir::Type* type = nullptr;
		switch (variadics.value().kind) {
			case VariadicKind::NORMAL: {
				break;
			}
			case VariadicKind::LEGACY: {
				kind = ir::VariadicsKind::LEGACY;
				break;
			}
			case VariadicKind::TYPED: {
				kind = ir::VariadicsKind::TYPED;
				type = variadics.value().type->emit(ctx);
				if (not(type->has_simple_copy() and type->has_simple_move())) {
					ctx->Error("Typed variadics require a type with simple-copy and simple-move, but got the type " +
					               ctx->color(type->to_string()) + " instead which does not satisfy that constraint",
					           variadics.value().type->fileRange);
				}
				break;
			}
		}
		variadicsIR = ir::Variadics{.kind = kind, .type = type};
	}
	ir::Type* retTy = returnType->emit(ctx);
	return ir::FunctionType::create(ir::ReturnType::get(retTy), irArgTys, variadicsIR, ctx->irCtx->llctx);
}

String FunctionType::to_string() const {
	String result("(");
	for (usize i = 0; i < argTypes.size(); i++) {
		result.append(argTypes[i]->to_string());
		if ((i != (argTypes.size() - 1)) || variadics.has_value()) {
			result.append(", ");
		}
	}
	if (variadics.has_value()) {
		switch (variadics.value().kind) {
			case VariadicKind::NORMAL: {
				result += "variadic";
				break;
			}
			case VariadicKind::LEGACY: {
				result += "variadic:legacy";
				break;
			}
			case VariadicKind::TYPED: {
				result += "variadic :: " + variadics.value().type->to_string();
				break;
			}
		}
	}
	result += ") -> " + returnType->to_string();
	return result;
}

} // namespace qat::ast
