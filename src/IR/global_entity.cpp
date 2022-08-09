#include "./global_entity.hpp"
#include "./qat_module.hpp"

namespace qat::IR {

GlobalEntity::GlobalEntity(QatModule *_parent, String _name, QatType *_type,
                           bool _is_variable, Value *_value,
                           utils::VisibilityInfo _visibility)
    : Value(nullptr, _type, _is_variable, Nature::assignable), name(_name),
      visibility(_visibility), initial(_value), parent(_parent) {
  // TODO
}

String GlobalEntity::getName() const { return name; }

String GlobalEntity::getFullName() const {
  return parent->getFullNameWithChild(name);
}

const utils::VisibilityInfo &GlobalEntity::getVisibility() const {
  return visibility;
}

bool GlobalEntity::hasInitial() const { return (initial != nullptr); }

u64 GlobalEntity::getLoadCount() const { return loads; }

u64 GlobalEntity::getStoreCount() const { return stores; }

u64 GlobalEntity::getReferCount() const { return refers; }

nuo::Json GlobalEntity::toJson() const {}

} // namespace qat::IR