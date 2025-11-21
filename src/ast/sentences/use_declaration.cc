#include "./use_declaration.hpp"
#include "../../IR/logic.hpp"
#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

void UseDeclaration::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent,
                                         EmitCtx* ctx) {
	UPDATE_DEPS(value);
}

ir::Value* UseDeclaration::emit(EmitCtx* ctx) {
	ir::Type* typRes = nullptr;
	if (type) {
		typRes = type->emit(ctx);
		if (not typRes->has_simple_copy()) {
			ctx->Error("The type provided for this use-value declaration is " + ctx->color(typRes->to_string()) +
			               ", which does not have simple-copy. Please check the logic",
			           type->fileRange);
		}
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(typRes);
		}
	}
	auto currBlock = ctx->get_fn()->get_block();
	if (currBlock->has_value(name.value)) {
		ctx->Error("Found a local value with the name " + ctx->color(name.value) + " in this scope", fileRange);
	} else if (currBlock->has_used_value(name.value)) {
		ctx->Error("Found a use-value declaration with the name " + ctx->color(name.value) + " in this scope",
		           fileRange);
	}
	auto val = value->emit(ctx);
	if (typRes && not val->get_ir_type()->is_same(typRes)) {
		ctx->Error("The type provided for this use-value declaration is " + ctx->color(typRes->to_string()) +
		               ", but the type of the value provided is " + ctx->color(val->get_ir_type()->to_string()),
		           fileRange);
	}
	if (not val->get_ir_type()->has_simple_copy()) {
		ctx->Error("The type of this expression is " + ctx->color(val->get_ir_type()->to_string()) +
		               ", which does not have simple-copy. In a use-value declaration, the type of the expression "
		               "should have simple-copy. This requirement is there to prevent object lifetime issues",
		           value->fileRange);
	}
	val = ir::Logic::handle_pass_semantics(ctx, val->get_pass_type(), val, fileRange);
	return currBlock->create_use_value(name.value, val->get_llvm(), val->get_ir_type(), ctx->irCtx, fileRange);
}

Json UseDeclaration::to_json() const {
	return Json()
	    ._("nodeType", "useDeclaration")
	    ._("name", name)
	    ._("value", value->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
