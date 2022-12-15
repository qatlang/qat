#include "./definition.hpp"
#include "../qat_module.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

DefinitionType::DefinitionType(Identifier _name, QatType* _subType, QatModule* _mod,
                               const utils::VisibilityInfo& _visibInfo)
    : EntityOverview("typeDefinition", Json(), _name.range), name(std::move(_name)), subType(_subType), parent(_mod),
      visibInfo(_visibInfo) {
  parent->typeDefs.push_back(this);
  llvmType = subType->getLLVMType();
}

utils::VisibilityInfo DefinitionType::getVisibility() const { return visibInfo; }

Identifier DefinitionType::getName() const { return name; }

String DefinitionType::getFullName() const { return parent ? parent->getFullNameWithChild(name.value) : name.value; }

QatModule* DefinitionType::getParent() { return parent; }

QatType* DefinitionType::getSubType() { return subType; }

void DefinitionType::updateOverview() {
  ovInfo._("fullName", getFullName())
      ._("typeID", getID())
      ._("subTypeID", subType->getID())
      ._("visibility", visibInfo)
      ._("moduleID", parent->getID());
}

TypeKind DefinitionType::typeKind() const { return TypeKind::definition; }

String DefinitionType::toString() const { return name.value; }

Json DefinitionType::toJson() const { return Json()._("type", "definition")._("subtype", subType->getID()); }

} // namespace qat::IR