#include "./type_like.hpp"
#include "../IR/type_id.hpp"
#include "../IR/value.hpp"
#include "./expression.hpp"
#include "./types/qat_type.hpp"

#include <llvm/IR/ConstantFold.h>

namespace qat::ast {

ir::Type* TypeLike::emit(EmitCtx* ctx) const {
	if (isType) {
		auto type = (Type*)data;
		return type->emit(ctx);
	} else {
		auto expr    = (PrerunExpression*)data;
		auto exprRes = expr->emit(ctx);
		if (not exprRes->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(exprRes->get_ir_type()->to_string()) + " instead",
			           expr->fileRange);
		};
		return ir::TypeInfo::get_for(exprRes->get_llvm_constant())->type;
	}
}
} // namespace qat::ast
