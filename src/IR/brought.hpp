#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include <functional>
#include <optional>

namespace qat::IR {

// Brought Entity
template <class T> class Brought {
  friend IR::QatModule;
  template <typename E> friend bool matchBroughtEntity(Brought<E> brought, String candName, Maybe<AccessInfo> reqInfo);

  // Optional name of the entity
  Maybe<Identifier> name;

  // The entity brought
  T* entity;

  // VisibilityInfo of the brought entity
  VisibilityInfo visibility;

public:
  Brought(Identifier _name, T* _entity, const VisibilityInfo& _visibility)
      : name(_name), entity(_entity), visibility(_visibility) {}

  Brought(T* _entity, const VisibilityInfo& _visibility) : entity(_entity), visibility(_visibility) {}

  // Get the name if the brought entity is named
  useit inline Identifier getName() const { return name.value_or(Identifier("", {""})); }

  // Is entity named
  useit inline bool isNamed() const { return name.has_value(); }

  // Get the entity
  useit inline T* get() const { return entity; }

  // Get the visibility of the brought entity
  useit inline const VisibilityInfo& getVisibility() const { return visibility; }
};

template <typename T> useit bool matchBroughtEntity(Brought<T> brought, String candName, Maybe<AccessInfo> reqInfo) {
  if (brought.isNamed()) {
    return (brought.name.value().value == candName) && brought.visibility.isAccessible(reqInfo) &&
           brought.entity->getVisibility().isAccessible(reqInfo);
  } else {
    return (brought.entity->getName().value == candName) && brought.visibility.isAccessible(reqInfo) &&
           brought.entity->getVisibility().isAccessible(reqInfo);
  }
}

} // namespace qat::IR

#endif