#include "./boolean_literal.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

ir::PrerunValue* BooleanLiteral::emit(EmitCtx* ctx) {
	return ir::PrerunValue::get(llvm::ConstantInt::getBool(ctx->irCtx->llctx, value),
	                            ir::UnsignedType::create_bool(ctx->irCtx));
}

String BooleanLiteral::to_string() const { return value ? "true" : "false"; }

} // namespace qat::ast
