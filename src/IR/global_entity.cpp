#include "./global_entity.hpp"
#include "./qat_module.hpp"

namespace qat {
namespace IR {

GlobalEntity::GlobalEntity(QatModule *_parent, std::string _name,
                           QatType *_type, bool _is_variable, Value *_value,
                           utils::VisibilityInfo _visibility)
    : parent(_parent), name(_name), initial(_value), visibility(_visibility),
      Value(_type, _is_variable, Kind::assignable) {}

std::string GlobalEntity::getName() const { return name; }

std::string GlobalEntity::getFullName() const {
  return parent->getFullNameWithChild(name);
}

const utils::VisibilityInfo &GlobalEntity::getVisibility() const {
  return visibility;
}

bool GlobalEntity::hasInitial() const { return (initial != nullptr); }

unsigned GlobalEntity::getLoadCount() const { return loads; }

unsigned GlobalEntity::getStoreCount() const { return stores; }

unsigned GlobalEntity::getReferCount() const { return refers; }

} // namespace IR
} // namespace qat