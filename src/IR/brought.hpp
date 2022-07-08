#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/visibility.hpp"
#include <optional>

namespace qat {
namespace IR {

// Brought Entity
template <class T> class Brought {
  // Optional name of the entity
  std::optional<std::string> name;

  // The entity brought
  T *entity;

  // VisibilityInfo of the brought entity
  utils::VisibilityInfo visibility;

public:
  Brought(std::string _name, T *_entity, utils::VisibilityInfo _visibility)
      : name(_name), entity(_entity), visibility(_visibility) {}

  Brought(T *_entity, utils::VisibilityInfo _visibility)
      : name(), entity(_entity), visibility(_visibility) {}

  // Get the name if the brought entity is named
  std::string getName() const { return name.value_or(""); }

  // Is entity named
  bool isNamed() { name.has_value(); }

  // Get the entity
  T *get() const { return entity; }

  // Get the visibility of the brought entity
  const utils::VisibilityInfo getVisibility() const { return visibility; }
};

} // namespace IR
} // namespace qat

#endif