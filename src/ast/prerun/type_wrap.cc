#include "./type_wrap.hpp"
#include "../../IR/type_id.hpp"
#include "../../IR/types/typed.hpp"

namespace qat::ast {

void TypeWrap::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
	theType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

ir::PrerunValue* TypeWrap::emit(EmitCtx* ctx) {
	return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, theType->emit(ctx), ctx->mod)->id,
	                            ir::TypedType::get(ctx->irCtx));
}

Json TypeWrap::to_json() const {
	return Json()
	    ._("nodeType", "typeWrap")
	    ._("providedType", theType->to_json())
	    ._("isExplicit", isExplicit)
	    ._("fileRange", fileRange);
}

String TypeWrap::to_string() const {
	return isExplicit ? ("type(" + theType->to_string() + ")") : theType->to_string();
}

} // namespace qat::ast
