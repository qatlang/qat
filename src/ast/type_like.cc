#include "./type_like.hpp"
#include "../IR/type_id.hpp"
#include "../IR/value.hpp"
#include "./expression.hpp"
#include "./types/qat_type.hpp"

#include <llvm/IR/ConstantFold.h>

namespace qat::ast {

String TypeLike::to_string() const {
	if (data) {
		if (kind == TypeLikeKind::TYPE) {
			return ((Type*)data)->to_string();
		} else if (kind == TypeLikeKind::PRERUN) {
			return ((PrerunExpression*)data)->to_string();
		}
	}
	return "";
}

TypeLike::operator JsonValue() const {
	if (data) {
		if (kind == TypeLikeKind::TYPE) {
			return ((Type*)data)->to_json();
		} else if (kind == TypeLikeKind::PRERUN) {
			return ((PrerunExpression*)data)->to_json();
		} else {
			return ((Expression*)data)->to_json();
		}
	}
	return JsonValue();
}

FileRange TypeLike::get_range() const {
	if (data) {
		if (kind == TypeLikeKind::TYPE) {
			return ((Type*)data)->fileRange;
		} else if (kind == TypeLikeKind::PRERUN) {
			return ((PrerunExpression*)data)->fileRange;
		} else {
			return ((Expression*)data)->fileRange;
		}
	}
	return FileRange("");
}

void TypeLike::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
	if (data) {
		if (kind == TypeLikeKind::TYPE) {
			((Type*)data)->update_dependencies(phase, dep, ent, ctx);
		} else if (kind == TypeLikeKind::PRERUN) {
			((PrerunExpression*)data)->update_dependencies(phase, dep, ent, ctx);
		} else if (kind == TypeLikeKind::EXPRESSION) {
			((Expression*)data)->update_dependencies(phase, dep, ent, ctx);
		}
	}
}

ir::Type* TypeLike::emit(EmitCtx* ctx) const {
	if (data == nullptr) {
		return nullptr;
	}
	if (kind == TypeLikeKind::TYPE) {
		auto type = (Type*)data;
		return type->emit(ctx);
	} else if (kind == TypeLikeKind::PRERUN) {
		auto expr    = (PrerunExpression*)data;
		auto exprRes = expr->emit(ctx);
		if (not exprRes->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(exprRes->get_ir_type()->to_string()) + " instead",
			           expr->fileRange);
		};
		return ir::TypeInfo::get_for(exprRes->get_llvm_constant())->type;
	} else {
		auto expr    = (Expression*)data;
		auto exprRes = expr->emit(ctx);
		if (not exprRes->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(exprRes->get_ir_type()->to_string()) + " instead",
			           expr->fileRange);
		}
		return ir::TypeInfo::get_for(exprRes->get_llvm_constant())->type;
	}
}
} // namespace qat::ast
