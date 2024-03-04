#include "./type_wrap.hpp"
#include "../../IR/types/typed.hpp"

namespace qat::ast {

void TypeWrap::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
  theType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

ir::PrerunValue* TypeWrap::emit(EmitCtx* ctx) {
  return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(theType->emit(ctx)));
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