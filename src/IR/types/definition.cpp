#include "./definition.hpp"
#include "../qat_module.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

DefinitionType::DefinitionType(Identifier _name, QatType* _subType, QatModule* _mod, const VisibilityInfo& _visibInfo)
    : EntityOverview("typeDefinition", Json(), _name.range), name(std::move(_name)), subType(_subType), parent(_mod),
      visibInfo(_visibInfo) {
  parent->typeDefs.push_back(this);
  llvmType = subType->getLLVMType();
}

VisibilityInfo DefinitionType::getVisibility() const { return visibInfo; }

bool DefinitionType::isExpanded() const { return subType->isExpanded(); }

bool DefinitionType::isDestructible() const { return subType->isDestructible(); }

void DefinitionType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  subType->destroyValue(ctx, vals, fun);
}

Identifier DefinitionType::getName() const { return name; }

String DefinitionType::getFullName() const { return parent ? parent->getFullNameWithChild(name.value) : name.value; }

QatModule* DefinitionType::getParent() { return parent; }

QatType* DefinitionType::getSubType() { return subType; }

bool DefinitionType::isTypeSized() const { return subType->isTypeSized(); }

void DefinitionType::updateOverview() {
  ovInfo._("fullName", getFullName())
      ._("typeID", getID())
      ._("subTypeID", subType->getID())
      ._("visibility", visibInfo)
      ._("moduleID", parent->getID());
}

bool DefinitionType::canBeConstGeneric() const { return subType->canBeConstGeneric(); }

Maybe<String> DefinitionType::toConstGenericString(IR::ConstantValue* constant) const {
  if (subType->canBeConstGeneric()) {
    return subType->toConstGenericString(constant);
  } else {
    return None;
  }
}

Maybe<bool> DefinitionType::equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const {
  if (subType->canBeConstGeneric()) {
    return subType->equalityOf(first, second);
  } else {
    return None;
  }
}

TypeKind DefinitionType::typeKind() const { return TypeKind::definition; }

String DefinitionType::toString() const { return getFullName(); }

} // namespace qat::IR