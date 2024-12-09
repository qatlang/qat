#include "./result.hpp"
#include "../../IR/types/result.hpp"
#include "qat_type.hpp"

namespace qat::ast {

void ResultType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                     EmitCtx* ctx) {
	validType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	errorType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

ir::Type* ResultType::emit(EmitCtx* ctx) {
	auto* validRes = validType->emit(ctx);
	if (validRes->is_opaque() && !validRes->as_opaque()->has_subtype() && !validRes->as_opaque()->has_deduced_size()) {
		ctx->Error("The valid value type " + ctx->color(validRes->to_string()) +
		               " provided is an incomplete type with unknown size",
		           validType->fileRange);
	} else if (!validRes->is_type_sized()) {
		ctx->Error("The valid value type " + ctx->color(validRes->to_string()) + " should be a sized type",
		           validType->fileRange);
	}
	auto* errorRes = errorType->emit(ctx);
	if (errorRes->is_opaque() && !errorRes->as_opaque()->has_subtype() && !errorRes->as_opaque()->has_deduced_size()) {
		ctx->Error("The error value type " + ctx->color(errorRes->to_string()) +
		               " provided is an incomplete type with unknown size",
		           errorType->fileRange);
	} else if (!errorRes->is_type_sized()) {
		ctx->Error("The error value type " + ctx->color(errorRes->to_string()) + " should be a sized type",
		           errorType->fileRange);
	}
	return ir::ResultType::get(validRes, errorRes, isPacked, ctx->irCtx);
}

AstTypeKind ResultType::type_kind() const { return AstTypeKind::RESULT; }

String ResultType::to_string() const {
	return "result:[" + String(isPacked ? "pack, " : "") + validType->to_string() + ", " + errorType->to_string() + "]";
}

Json ResultType::to_json() const {
	return Json()
	    ._("typeKind", "result")
	    ._("validType", validType->to_json())
	    ._("errorType", errorType->to_json())
	    ._("isPacked", isPacked)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast