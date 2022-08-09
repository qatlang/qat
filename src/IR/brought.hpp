#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/visibility.hpp"
#include <optional>

namespace qat::IR {

// Brought Entity
template <class T> class Brought {
  // Optional name of the entity
  Maybe<String> name;

  // The entity brought
  T *entity;

  // VisibilityInfo of the brought entity
  utils::VisibilityInfo visibility;

public:
  Brought(String _name, T *_entity, const utils::VisibilityInfo &_visibility)
      : name(_name), entity(_entity), visibility(_visibility) {}

  Brought(T *_entity, const utils::VisibilityInfo &_visibility)
      : entity(_entity), visibility(_visibility) {}

  // Get the name if the brought entity is named
  useit String getName() const { return name.value_or(""); }

  // Is entity named
  useit bool isNamed() { return name.has_value(); }

  // Get the entity
  useit T *get() const { return entity; }

  // Get the visibility of the brought entity
  useit utils::VisibilityInfo getVisibility() const { return visibility; }
};

} // namespace qat::IR

#endif