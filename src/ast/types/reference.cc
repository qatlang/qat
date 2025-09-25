#include "./reference.hpp"
#include "../../IR/types/reference.hpp"
#include "../expression.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

void RefType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	type->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	if (addressSpace.has_value() && addressSpace.value().value) {
		UPDATE_DEPS(addressSpace.value().value);
	}
}

Maybe<usize> RefType::get_type_bitsize(EmitCtx* ctx) const {
	if (addressSpace.has_value() && addressSpace.value().value) {
		return None;
	}
	return (usize)(ctx->irCtx->dataLayout.getTypeAllocSizeInBits(llvm::PointerType::get(
	    ctx->irCtx->llctx, addressSpace.has_value() ? addressSpace.value().to_ir(ctx).get_number(ctx->irCtx)
	                                                : ctx->irCtx->dataLayout.getProgramAddressSpace())));
}

ir::Type* RefType::emit(EmitCtx* ctx) {
	auto* typRes = type->emit(ctx);
	if (typRes->is_ref() || typRes->is_void() || typRes->is_region()) {
		ctx->Error("Sub-type of reference cannot be " + ctx->color(typRes->to_string()), fileRange);
	}
	Maybe<ir::AddressSpace> addr;
	if (addressSpace.has_value()) {
		addr = addressSpace.value().to_ir(ctx);
	}
	return ir::RefType::get(isSubtypeVar, typRes, std::move(addr), ctx->irCtx);
}

AstTypeKind RefType::type_kind() const { return AstTypeKind::REFERENCE; }

Json RefType::to_json() const {
	return Json()
	    ._("typeKind", "reference")
	    ._("subType", type->to_json())
	    ._("isSubtypeVariable", isSubtypeVar)
	    ._("hasAddressSpace", addressSpace.has_value())
	    ._("addressSpace", addressSpace.has_value() ? addressSpace.value().to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

String RefType::to_string() const {
	return "ref:[" + String(isSubtypeVar ? "var " : "") + type->to_string() +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ast
