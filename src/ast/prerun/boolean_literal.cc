#include "./boolean_literal.hpp"

namespace qat::ast {

ir::PrerunValue* BooleanLiteral::emit(EmitCtx* ctx) {
	return ir::PrerunValue::get(llvm::ConstantInt::getBool(ctx->irCtx->llctx, value),
	                            ir::UnsignedType::create_bool(ctx->irCtx));
}

String BooleanLiteral::to_string() const { return value ? "true" : "false"; }

Json BooleanLiteral::to_json() const {
	return Json()._("nodeType", "booleanLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast
