#include "./static_member.hpp"
#include "./types/core_type.hpp"

namespace qat {
namespace IR {

StaticMember::StaticMember(CoreType *_parent, std::string _name, QatType *_type,
                           bool _isVariable, Value *_initial,
                           utils::VisibilityInfo _visibility)
    : parent(_parent), name(_name), initial(_initial), visibility(_visibility),
      Value(_type, _isVariable, Kind::assignable) {}

CoreType *StaticMember::getParentType() { return parent; }

std::string StaticMember::getName() const { return name; }

std::string StaticMember::getFullName() const {
  return parent->getFullName() + ":" + name;
}

const utils::VisibilityInfo &StaticMember::getVisibility() const {
  return visibility;
}

bool StaticMember::hasInitial() const { return (initial != nullptr); }

unsigned StaticMember::getLoadCount() const { return loads; }

unsigned StaticMember::getStoreCount() const { return stores; }

unsigned StaticMember::getReferCount() const { return refers; }

} // namespace IR
} // namespace qat