#include "./opaque.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"
#include "./string_slice.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

OpaqueType::OpaqueType(Identifier _name, Vec<GenericParameter*> _generics, Maybe<String> _genericID,
                       Maybe<OpaqueSubtypeKind> _subtypeKind, IR::QatModule* _parent, Maybe<usize> _size,
                       VisibilityInfo _visibility, llvm::LLVMContext& llctx, Maybe<MetaInfo> _metaInfo)
    : EntityOverview(_subtypeKind.has_value()
                         ? (_subtypeKind.value() == OpaqueSubtypeKind::core
                                ? "coreType"
                                : (_subtypeKind.value() == OpaqueSubtypeKind::mix ? "mixType" : "opaqueType"))
                         : "opaqueType",
                     Json(), _name.range),
      name(std::move(_name)), generics(std::move(_generics)), genericID(_genericID),
      subtypeKind(std::move(_subtypeKind)), parent(_parent), size(_size), visibility(_visibility), metaInfo(_metaInfo) {
  Maybe<String> foreignID;
  if (metaInfo) {
    if (metaInfo->hasKey("foreign")) {
      foreignID = metaInfo->getForeignID();
    }
  }
  auto linkNames = parent->getLinkNames().newWith(
      LinkNameUnit(name.value, (subtypeKind.has_value() && subtypeKind.value() == OpaqueSubtypeKind::mix)
                                   ? LinkUnitType::mix
                                   : LinkUnitType::type),
      foreignID);
  if (isGeneric()) {
    Vec<LinkNames> genericLinkNames;
    for (auto* param : generics) {
      if (param->isTyped()) {
        genericLinkNames.push_back(
            LinkNames({LinkNameUnit(param->asTyped()->getType()->getNameForLinking(), LinkUnitType::genericTypeValue)},
                      None, nullptr));
      } else if (param->isPrerun()) {
        auto preRes = param->asPrerun();
        genericLinkNames.push_back(
            LinkNames({LinkNameUnit(preRes->getType()->toPrerunGenericString(preRes->getExpression()).value(),
                                    LinkUnitType::genericPrerunValue)},
                      None, nullptr));
      }
    }
    linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, None, genericLinkNames), None);
  }
  linkingName = linkNames.toName();
  llvmType    = llvm::StructType::create(llctx, linkingName);
  if (!isGeneric()) {
    parent->opaqueTypes.push_back(this);
  }
}

OpaqueType* OpaqueType::get(Identifier name, Vec<GenericParameter*> generics, Maybe<String> genericID,
                            Maybe<OpaqueSubtypeKind> subtypeKind, IR::QatModule* parent, Maybe<usize> size,
                            VisibilityInfo visibility, llvm::LLVMContext& llCtx, Maybe<MetaInfo> metaInfo) {
  return new OpaqueType(std::move(name), std::move(generics), genericID, subtypeKind, parent, size, visibility, llCtx,
                        metaInfo);
}

String OpaqueType::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier OpaqueType::getName() const { return name; }

bool OpaqueType::isGeneric() const { return !generics.empty(); }

Maybe<String> OpaqueType::getGenericID() const { return genericID; }

bool OpaqueType::hasGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->isSame(name)) {
      return true;
    }
  }
  return false;
}

GenericParameter* OpaqueType::getGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->isSame(name)) {
      return gen;
    }
  }
  return nullptr;
}

VisibilityInfo const& OpaqueType::getVisibility() const { return visibility; }

bool OpaqueType::isSubtypeCore() const { return subtypeKind == OpaqueSubtypeKind::core; }

bool OpaqueType::isSubtypeMix() const { return subtypeKind == OpaqueSubtypeKind::mix; }

bool OpaqueType::isSubtypeUnknown() const { return subtypeKind == OpaqueSubtypeKind::unknown; }

bool OpaqueType::hasSubType() const { return subTy != nullptr; }

void OpaqueType::setSubType(IR::ExpandedType* _subTy) {
  subTy = _subTy;
  SHOW("Opaque: set subtype")
  if (!isGeneric()) {
    for (auto item = parent->opaqueTypes.begin(); item != parent->opaqueTypes.end(); item++) {
      if ((*item)->getID() == getID()) {
        parent->opaqueTypes.erase(item);
        break;
      }
    }
  }
}

IR::QatType* OpaqueType::getSubType() const { return subTy; }

bool OpaqueType::hasDeducedSize() const { return size.has_value(); }

usize OpaqueType::getDeducedSize() const { return size.value(); }

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

bool OpaqueType::isTypeSized() const { return subTy ? subTy->isTypeSized() : size.has_value(); }

bool OpaqueType::isTriviallyCopyable() const { return subTy && subTy->isTriviallyCopyable(); }

bool OpaqueType::isTriviallyMovable() const { return subTy && subTy->isTriviallyMovable(); }

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