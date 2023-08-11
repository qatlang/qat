#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include <optional>

namespace qat::IR {

// Brought Entity
template <class T> class Brought {
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
  useit Identifier getName() const { return name.value_or(Identifier("", {""})); }

  // Is entity named
  useit bool isNamed() const { return name.has_value(); }

  // Get the entity
  useit T* get() const { return entity; }

  // Get the visibility of the brought entity
  useit const VisibilityInfo& getVisibility() const { return visibility; }
};

} // namespace qat::IR

#endif