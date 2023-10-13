#include "./generics.hpp"
#include "expression.hpp"
#include "node.hpp"

namespace qat::ast {

FillGeneric::FillGeneric(QatType* type)
    : data(type), kind(FillGenericKind::typed){SHOW("FillGeneric created for type " << type->toString())}

      FillGeneric::FillGeneric(PrerunExpression * constant)
    : data(constant), kind(FillGenericKind::constant) {
  SHOW("FillGeneric created for const" << constant->toString())
}

bool FillGeneric::isType() const { return kind == FillGenericKind::typed; }

bool FillGeneric::isConst() const { return kind == FillGenericKind::constant; }

QatType* FillGeneric::asType() const { return (QatType*)data; }

PrerunExpression* FillGeneric::asConst() const { return (PrerunExpression*)data; }

FileRange const& FillGeneric::getRange() const { return asType()->fileRange; }

IR::GenericToFill* FillGeneric::toFill(IR::Context* ctx) const {
  SHOW("ToFill called")
  if (isType()) {
    return IR::GenericToFill::get(asType()->emit(ctx), asType()->fileRange);
  } else {
    return IR::GenericToFill::get(asConst()->emit(ctx), asConst()->fileRange);
  }
}

String FillGeneric::toString() const {
  if (isType()) {
    return asType()->toString();
  } else {
    return asConst()->toString();
  }
}

Json FillGeneric::toJson() const {
  return Json()
      ._("kind", (kind == FillGenericKind::typed) ? "typed" : "const")
      ._("type", isType() ? asType()->toJson() : Json())
      ._("constant", isConst() ? asConst()->toJson() : Json());
}

} // namespace qat::ast