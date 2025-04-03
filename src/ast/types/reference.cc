#include "./reference.hpp"
#include "../../IR/types/reference.hpp"
#include "../expression.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

void ReferenceType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                        EmitCtx* ctx) {
	type->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

Maybe<usize> ReferenceType::get_type_bitsize(EmitCtx* ctx) const {
	return (usize)(ctx->irCtx->dataLayout.getTypeAllocSizeInBits(
	    llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u)));
}

ir::Type* ReferenceType::emit(EmitCtx* ctx) {
	auto* typRes = type->emit(ctx);
	if (typRes->is_ref() || typRes->is_void() || typRes->is_region()) {
		ctx->Error("Sub-type of reference cannot be " + ctx->color(typRes->to_string()), fileRange);
	}
	return ir::RefType::get(isSubtypeVar, typRes, ctx->irCtx);
}

AstTypeKind ReferenceType::type_kind() const { return AstTypeKind::REFERENCE; }

Json ReferenceType::to_json() const {
	return Json()
	    ._("typeKind", "reference")
	    ._("subType", type->to_json())
	    ._("isSubtypeVariable", isSubtypeVar)
	    ._("fileRange", fileRange);
}

String ReferenceType::to_string() const {
	return "ref:[" + String(isSubtypeVar ? "var " : "") + type->to_string() + "]";
}

} // namespace qat::ast
