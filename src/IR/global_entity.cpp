#include "./global_entity.hpp"
#include "./qat_module.hpp"
#include "entity_overview.hpp"

namespace qat::IR {

GlobalEntity::GlobalEntity(QatModule* _parent, Identifier _name, QatType* _type, bool _is_variable,
                           Maybe<llvm::Constant*> _initialValue, llvm::Value* _value,
                           const utils::VisibilityInfo& _visibility)
    : Value(_value, _type, _is_variable, Nature::assignable), EntityOverview("global", Json(), _name.range),
      name(std::move(_name)), visibility(_visibility), parent(_parent), initialValue(_initialValue) {
  parent->globalEntities.push_back(this);
}

Identifier GlobalEntity::getName() const { return name; }

String GlobalEntity::getFullName() const { return parent->getFullNameWithChild(name.value); }

bool GlobalEntity::hasInitialValue() const { return initialValue.has_value(); }

llvm::Constant* GlobalEntity::getInitialValue() const { return initialValue.value(); }

const utils::VisibilityInfo& GlobalEntity::getVisibility() const { return visibility; }

u64 GlobalEntity::getLoadCount() const { return loads; }

u64 GlobalEntity::getStoreCount() const { return stores; }

u64 GlobalEntity::getReferCount() const { return refers; }

} // namespace qat::IR