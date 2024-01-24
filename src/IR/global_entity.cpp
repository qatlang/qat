#include "./global_entity.hpp"
#include "./qat_module.hpp"
#include "entity_overview.hpp"

namespace qat::IR {

PrerunGlobal::PrerunGlobal(QatModule* _parent, Identifier _name, QatType* _type, llvm::Constant* _constant,
                           VisibilityInfo _visibility, FileRange _fileRange)
    : PrerunValue(_constant, _type), EntityOverview("prerunGlobal", Json(), _fileRange), name(_name),
      visibility(_visibility), parent(_parent) {
  parent->prerunGlobals.push_back(this);
}

String PrerunGlobal::getFullName() const { return parent->getFullNameWithChild(name.value); }

GlobalEntity::GlobalEntity(QatModule* _parent, Identifier _name, QatType* _type, bool _is_variable,
                           Maybe<llvm::Constant*> _initialValue, llvm::Value* _value, const VisibilityInfo& _visibility)
    : Value(_value, _type, _is_variable, Nature::assignable), EntityOverview("global", Json(), _name.range),
      name(std::move(_name)), visibility(_visibility), parent(_parent), initialValue(_initialValue) {
  parent->globalEntities.push_back(this);
}

IR::QatModule* GlobalEntity::getParent() const { return parent; }

Identifier GlobalEntity::getName() const { return name; }

String GlobalEntity::getFullName() const { return parent->getFullNameWithChild(name.value); }

bool GlobalEntity::hasInitialValue() const { return initialValue.has_value(); }

llvm::Constant* GlobalEntity::getInitialValue() const { return initialValue.value(); }

const VisibilityInfo& GlobalEntity::getVisibility() const { return visibility; }

} // namespace qat::IR