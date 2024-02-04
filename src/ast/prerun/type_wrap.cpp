#include "./type_wrap.hpp"
#include "../../IR/types/typed.hpp"

namespace qat::ast {

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

String TypeWrap::toString() const { return "type(" + theType->toString() + ")"; }

} // namespace qat::ast