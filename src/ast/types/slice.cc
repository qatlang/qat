#include "./slice.hpp"
#include "../../IR/types/slice.hpp"

namespace qat::ast {

ir::Type* SliceType::emit(EmitCtx* ctx) {
	Maybe<ir::AddressSpace> addr;
	if (addressSpace.has_value()) {
		addr = addressSpace.value().to_ir(ctx);
	}
	return ir::SliceType::get(isVar, subType->emit(ctx), std::move(addr), ctx->irCtx);
}

String SliceType::to_string() const {
	return "slice:[" + String(isVar ? "var " : "") + subType->to_string() +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ast
