#include "./static_member.hpp"
#include "./qat_module.hpp"
#include "./types/core_type.hpp"
#include "entity_overview.hpp"

namespace qat::IR {

StaticMember::StaticMember(CoreType* _parent, Identifier _name, QatType* _type, bool _isVariable, Value* _initial,
                           const VisibilityInfo& _visibility)
    : Value(nullptr, _type, _isVariable, Nature::assignable), EntityOverview("staticMember", Json(), _name.range),
      name(std::move(_name)), parent(_parent), initial(_initial), loads(0), stores(0), refers(0),
      visibility(_visibility) {
  // TODO
}

CoreType* StaticMember::getParentType() { return parent; }

Identifier StaticMember::getName() const { return name; }

String StaticMember::getFullName() const { return parent->getFullName() + ":" + name.value; }

const VisibilityInfo& StaticMember::getVisibility() const { return visibility; }

bool StaticMember::hasInitial() const { return (initial != nullptr); }

u64 StaticMember::getLoadCount() const { return loads; }

u64 StaticMember::getStoreCount() const { return stores; }

u64 StaticMember::getReferCount() const { return refers; }

Json StaticMember::toJson() const {
  // TODO - Implement
  return Json();
}

void StaticMember::updateOverview() {
  ovInfo._("parentID", parent->getID())
      ._("typeID", getType()->getID())
      ._("type", type->toString())
      ._("name", name)
      ._("fullName", getFullName())
      ._("visibility", visibility)
      ._("isVariable", isVariable())
      ._("moduleID", parent->getParent()->getID());
}

} // namespace qat::IR