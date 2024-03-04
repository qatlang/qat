#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include <functional>
#include <optional>

namespace qat::ir {

// Brought Entity
template <class T> class Brought {
  friend ir::Mod;
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
  useit inline Identifier get_name() const { return name.value_or(Identifier("", {""})); }

  // Is entity named
  useit inline bool is_named() const { return name.has_value(); }

  // Get the entity
  useit inline T* get() const { return entity; }

  // Get the visibility of the brought entity
  useit inline const VisibilityInfo& get_visibility() const { return visibility; }
};

template <typename T> useit bool matchBroughtEntity(Brought<T> brought, String candName, Maybe<AccessInfo> reqInfo) {
  if (brought.is_named()) {
    return (brought.name.value().value == candName) && brought.visibility.is_accessible(reqInfo) &&
           brought.entity->get_visibility().is_accessible(reqInfo);
  } else {
    return (brought.entity->get_name().value == candName) && brought.visibility.is_accessible(reqInfo) &&
           brought.entity->get_visibility().is_accessible(reqInfo);
  }
}

} // namespace qat::ir

#endif