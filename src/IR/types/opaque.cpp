#include "./opaque.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

OpaqueType::OpaqueType(Identifier _name, Maybe<OpaqueSubtypeKind> _subtypeKind, IR::QatModule* _parent,
                       Maybe<usize> _size, VisibilityInfo _visibility, llvm::LLVMContext& llctx)
    : EntityOverview(_subtypeKind.has_value()
                         ? (_subtypeKind.value() == OpaqueSubtypeKind::core
                                ? "coreType"
                                : (_subtypeKind.value() == OpaqueSubtypeKind::mix ? "mixType" : "opaqueType"))
                         : "opaqueType",
                     Json(), _name.range),
      name(std::move(_name)), subtypeKind(std::move(_subtypeKind)), parent(_parent), size(_size),
      visibility(_visibility) {
  llvmType = llvm::StructType::create(llctx, parent->getFullNameWithChild(name.value));
  parent->opaqueTypes.push_back(this);
}

OpaqueType* OpaqueType::get(Identifier name, Maybe<OpaqueSubtypeKind> subtypeKind, IR::QatModule* parent,
                            Maybe<usize> size, VisibilityInfo visibility, llvm::LLVMContext& llCtx) {
  return new OpaqueType(std::move(name), subtypeKind, parent, size, visibility, llCtx);
}

String OpaqueType::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier OpaqueType::getName() const { return name; }

VisibilityInfo const& OpaqueType::getVisibility() const { return visibility; }

bool OpaqueType::isSubtypeCore() const { return subtypeKind == OpaqueSubtypeKind::core; }

bool OpaqueType::isSubtypeMix() const { return subtypeKind == OpaqueSubtypeKind::mix; }

bool OpaqueType::isSubtypeUnknown() const { return subtypeKind == OpaqueSubtypeKind::unknown; }

bool OpaqueType::hasSubType() const { return subTy != nullptr; }

void OpaqueType::setSubType(IR::ExpandedType* _subTy) {
  subTy = _subTy;
  SHOW("Opaque: set subtype")
  for (auto item = parent->opaqueTypes.begin(); item != parent->opaqueTypes.end(); item++) {
    if ((*item)->getID() == getID()) {
      parent->opaqueTypes.erase(item);
      break;
    }
  }
}

IR::QatType* OpaqueType::getSubType() const { return subTy; }

bool OpaqueType::hasSize() const { return size.has_value(); }

bool OpaqueType::isExpanded() const { return subTy && subTy->isExpanded(); }

bool OpaqueType::hasNoValueSemantics() const { return subTy && subTy->hasNoValueSemantics(); }

bool OpaqueType::canBePrerunGeneric() const { return subTy && subTy->canBePrerunGeneric(); }

Maybe<String> OpaqueType::toPrerunGenericString(IR::PrerunValue* preVal) const {
  if (subTy) {
    return subTy->toPrerunGenericString(preVal);
  } else {
    // FIXME - Throw error?
    return None;
  }
}

bool OpaqueType::isTypeSized() const { subTy ? subTy->isTypeSized() : size.has_value(); }

Maybe<bool> OpaqueType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (subTy) {
    return subTy->equalityOf(first, second);
  } else {
    return None;
  }
}

bool OpaqueType::isCopyConstructible() const { return subTy && subTy->isCopyConstructible(); }

bool OpaqueType::isCopyAssignable() const { return subTy && subTy->isCopyAssignable(); }

bool OpaqueType::isMoveConstructible() const { return subTy && subTy->isMoveConstructible(); }

bool OpaqueType::isMoveAssignable() const { return subTy && subTy->isMoveAssignable(); }

bool OpaqueType::isDestructible() const { return subTy && subTy->isDestructible(); }

void OpaqueType::copyConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->copyConstructValue(ctx, vals, fun);
  }
}

void OpaqueType::copyAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->copyAssignValue(ctx, vals, fun);
  }
}

void OpaqueType::moveConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->moveConstructValue(ctx, vals, fun);
  }
}

void OpaqueType::moveAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->moveAssignValue(ctx, vals, fun);
  }
}

void OpaqueType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->destroyValue(ctx, vals, fun);
  }
  // FIXME - Evaluate if an error should be thrown if there is no subtype set
}

TypeKind OpaqueType::typeKind() const { return TypeKind::opaque; }

String OpaqueType::toString() const { return getFullName(); }

} // namespace qat::IR