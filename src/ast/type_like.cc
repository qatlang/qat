#include "./type_like.hpp"
#include "../IR/type_id.hpp"
#include "../IR/value.hpp"
#include "./expression.hpp"
#include "./types/qat_type.hpp"

#include <llvm/IR/ConstantFold.h>

namespace qat::ast {

String TypeLike::to_string() const {
	return data ? (isType ? ((Type*)data)->to_string() : ((PrerunExpression*)data)->to_string()) : "";
}

JsonValue TypeLike::to_json_value() const {
	return data ? (isType ? ((Type*)data)->to_json() : ((PrerunExpression*)data)->to_json()) : JsonValue();
}

FileRange TypeLike::get_range() const {
	return data ? (isType ? ((Type*)data)->fileRange : ((PrerunExpression*)data)->fileRange) : FileRange("");
}

void TypeLike::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
	if (data) {
		if (isType) {
			((Type*)data)->update_dependencies(phase, dep, ent, ctx);
		} else {
			((PrerunExpression*)data)->update_dependencies(phase, dep, ent, ctx);
		}
	}
}

ir::Type* TypeLike::emit(EmitCtx* ctx) const {
	if (data == nullptr) {
		return nullptr;
	}
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
