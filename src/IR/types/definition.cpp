#include "./definition.hpp"
#include "../qat_module.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

DefinitionType::DefinitionType(String _name, QatType *_subType, QatModule *_mod,
                               const utils::VisibilityInfo &_visibInfo)
    : name(std::move(_name)), subType(_subType), parent(_mod),
      visibInfo(_visibInfo) {
  parent->typeDefs.push_back(this);
  llvmType = subType->getLLVMType();
}

utils::VisibilityInfo DefinitionType::getVisibility() const {
  return visibInfo;
}

String DefinitionType::getName() const { return name; }

String DefinitionType::getFullName() const {
  return parent ? parent->getFullNameWithChild(name) : name;
}

QatModule *DefinitionType::getParent() { return parent; }

QatType *DefinitionType::getSubType() { return subType; }

TypeKind DefinitionType::typeKind() const { return TypeKind::definition; }

String DefinitionType::toString() const { return name; }

Json DefinitionType::toJson() const {
  return Json()._("type", "definition")._("subtype", subType->getID());
}

} // namespace qat::IR