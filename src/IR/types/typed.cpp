#include "./typed.hpp"
#include "../../memory_tracker.hpp"
#include "../value.hpp"
#include "type_kind.hpp"

namespace qat::IR {

TypedType::TypedType(IR::QatType* _subTy) : subTy(_subTy) {
  while (subTy->isTyped()) {
    subTy = subTy->asTyped()->getSubType();
  }
  llvmType    = subTy->getLLVMType();
  linkingName = "type(" + subTy->getNameForLinking() + ")";
}

TypedType* TypedType::get(QatType* _subTy) {
  for (auto* typ : types) {
    if (typ->isTyped() && typ->asTyped()->getSubType()->getID() == _subTy->getID()) {
      return typ->asTyped();
    }
  }
  return new TypedType(_subTy);
}

IR::QatType* TypedType::getSubType() const { return subTy->isTyped() ? subTy->asTyped()->getSubType() : subTy; }

TypeKind TypedType::typeKind() const { return TypeKind::typed; }

String TypedType::toString() const { return subTy->toString(); }

Maybe<bool> TypedType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  return first->getType()->asTyped()->getSubType()->isSame(second->getType()->asTyped()->getSubType());
}

bool TypedType::canBePrerunGeneric() const { return true; }

Maybe<String> TypedType::toPrerunGenericString(IR::PrerunValue* val) const {
  // FIXME - The following is stupid, if there is confirmation that the constant's type is already checked
  return val->getType()->asTyped()->getSubType()->toString();
}

} // namespace qat::IR