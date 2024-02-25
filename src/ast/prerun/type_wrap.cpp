#include "./type_wrap.hpp"
#include "../../IR/types/typed.hpp"

namespace qat::ast {

void TypeWrap::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                                   IR::Context* ctx) {
  theType->update_dependencies(phase, IR::DependType::complete, ent, ctx);
}

IR::PrerunValue* TypeWrap::emit(IR::Context* ctx) {
  return new IR::PrerunValue(IR::TypedType::get(theType->emit(ctx)));
}

Json TypeWrap::toJson() const {
  return Json()
      ._("nodeType", "typeWrap")
      ._("providedType", theType->toJson())
      ._("isExplicit", isExplicit)
      ._("fileRange", fileRange);
}

String TypeWrap::toString() const { return isExplicit ? ("type(" + theType->toString() + ")") : theType->toString(); }

} // namespace qat::ast