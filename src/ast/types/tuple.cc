#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat::ast {

void TupleType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                    EmitCtx* ctx) {
	for (auto typ : types) {
		typ->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
}

Maybe<usize> TupleType::getTypeSizeInBits(EmitCtx* ctx) const {
	usize total = 0;
	for (auto* typ : types) {
		auto typSiz = typ->getTypeSizeInBits(ctx);
		if (typSiz.has_value()) {
			total += typSiz.value();
		} else {
			return None;
		}
	}
	return total;
}

ir::Type* TupleType::emit(EmitCtx* ctx) {
	Vec<ir::Type*> irTypes;
	for (auto* type : types) {
		if (type->type_kind() == ast::AstTypeKind::VOID) {
			ctx->Error("Tuple cannot contain a member of type " + ctx->color("void"), fileRange);
		}
		irTypes.push_back(type->emit(ctx));
	}
	return ir::TupleType::get(irTypes, isPacked, ctx->irCtx->llctx);
}

AstTypeKind TupleType::type_kind() const { return AstTypeKind::TUPLE; }

Json TupleType::to_json() const {
	Vec<JsonValue> mems;
	for (auto* mem : types) {
		mems.push_back(mem->to_json());
	}
	return Json()._("typeKind", "tuple")._("members", mems)._("isPacked", isPacked)._("fileRange", fileRange);
}

String TupleType::to_string() const {
	String result;
	if (isPacked) {
		result += "pack ";
	}
	result = "(";
	for (usize i = 0; i < types.size(); i++) {
		result += types.at(i)->to_string();
		if (i != (types.size() - 1)) {
			result += "; ";
		}
	}
	result += ")";
	return result;
}

} // namespace qat::ast