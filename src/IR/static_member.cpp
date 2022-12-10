#include "./static_member.hpp"
#include "./types/core_type.hpp"

namespace qat::IR {

StaticMember::StaticMember(CoreType* _parent, Identifier _name, QatType* _type, bool _isVariable, Value* _initial,
                           const utils::VisibilityInfo& _visibility)
    : Value(nullptr, _type, _isVariable, Nature::assignable), name(std::move(_name)), parent(_parent),
      initial(_initial), loads(0), stores(0), refers(0), visibility(_visibility) {
  // TODO
}

CoreType* StaticMember::getParentType() { return parent; }

Identifier StaticMember::getName() const { return name; }

String StaticMember::getFullName() const { return parent->getFullName() + ":" + name.value; }

const utils::VisibilityInfo& StaticMember::getVisibility() const { return visibility; }

bool StaticMember::hasInitial() const { return (initial != nullptr); }

u64 StaticMember::getLoadCount() const { return loads; }

u64 StaticMember::getStoreCount() const { return stores; }

u64 StaticMember::getReferCount() const { return refers; }

Json StaticMember::toJson() const {
  // TODO - Implement
}

} // namespace qat::IR