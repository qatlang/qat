#include "./error.hpp"
#include "../../IR/types/error.hpp"

namespace qat::ast {

ir::Type* ErrorType::emit(EmitCtx* ctx) {
	auto subTy = subType->emit(ctx);
	if (subTy->is_opaque()) {
		ctx->Error("The underlying type of an " + ctx->color("error") + " type can't be an opaque type. Got " +
		               ctx->color(subTy->to_string()) +
		               " as the underlying type for this error type. Error types are transparent in qat, meaning that"
		               " error types take the exact form of their underlying type, without any nesting or redirection",
		           fileRange);
	}
	if (not subTy->is_type_sized()) {
		ctx->Error("The underlying type of an " + ctx->color("error") + " type should have a known size. Got " +
		               ctx->color(subTy->to_string()) +
		               " as the underlying type for this error type. Error types are transparent in qat, meaning that "
		               "error types take the exact form of their underlying type, without any nesting or redirection",
		           fileRange);
	}
	return ir::ErrorType::get(hasNoneVariant, subTy);
}

} // namespace qat::ast
