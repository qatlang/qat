#include "./slice.hpp"
#include "../../IR/types/slice.hpp"

namespace qat::ast {

ir::Type* SliceType::emit(EmitCtx* ctx) { return ir::SliceType::get(isVar, subType->emit(ctx), ctx->irCtx); }

Json SliceType::to_json() const {
	return Json()._("nodeType", "slice")._("isVar", isVar)._("subType", subType->to_string())._("fileRange", fileRange);
}

String SliceType::to_string() const { return "slice:[" + String(isVar ? "var " : "") + subType->to_string() + "]"; }

} // namespace qat::ast
