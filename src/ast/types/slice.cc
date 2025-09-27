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

Json SliceType::to_json() const {
	return Json()
	    ._("nodeType", "slice")
	    ._("isVar", isVar)
	    ._("subType", subType->to_string())
	    ._("hasAddressSpace", addressSpace.has_value())
	    ._("addressSpace", addressSpace.has_value() ? addressSpace.value().to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

String SliceType::to_string() const {
	return "slice:[" + String(isVar ? "var " : "") + subType->to_string() +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ast
