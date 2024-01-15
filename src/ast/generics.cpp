#include "./generics.hpp"
#include "expression.hpp"
#include "node.hpp"
#include "types/integer.hpp"
#include "types/unsigned.hpp"

namespace qat::ast {

FillGeneric::FillGeneric(QatType* type)
    : data(type), kind(FillGenericKind::typed){SHOW("FillGeneric created for type " << type->toString())}

      FillGeneric::FillGeneric(PrerunExpression * constant)
    : data(constant), kind(FillGenericKind::prerun) {
  SHOW("FillGeneric created for const" << constant->toString())
}

bool FillGeneric::isType() const { return kind == FillGenericKind::typed; }

bool FillGeneric::isPrerun() const { return kind == FillGenericKind::prerun; }

QatType* FillGeneric::asType() const { return (QatType*)data; }

PrerunExpression* FillGeneric::asPrerun() const { return (PrerunExpression*)data; }

FileRange const& FillGeneric::getRange() const { return asType()->fileRange; }

IR::GenericToFill* FillGeneric::toFill(IR::Context* ctx) const {
  SHOW("ToFill called")
  if (isType()) {
    if (asType()->typeKind() == TypeKind::integer) {
      ((IntegerType*)asType())->isPartOfGeneric = true;
    } else if (asType()->typeKind() == TypeKind::unsignedInteger) {
      ((UnsignedType*)asType())->isPartOfGeneric = true;
    }
    return IR::GenericToFill::GetType(asType()->emit(ctx), asType()->fileRange);
  } else {
    return IR::GenericToFill::GetPrerun(asPrerun()->emit(ctx), asPrerun()->fileRange);
  }
}

String FillGeneric::toString() const {
  if (isType()) {
    return asType()->toString();
  } else {
    return asPrerun()->toString();
  }
}

Json FillGeneric::toJson() const {
  return Json()
      ._("kind", (kind == FillGenericKind::typed) ? "typed" : "prerun")
      ._("type", isType() ? asType()->toJson() : Json())
      ._("constant", isPrerun() ? asPrerun()->toJson() : Json());
}

} // namespace qat::ast