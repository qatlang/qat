#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/visibility.hpp"
#include "./value.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class QatModule;

class GlobalEntity : public Value {
private:
  // Name of this GlobalEntity
  std::string name;

  // Details about the visibility of this global
  utils::VisibilityInfo visibility;

  Value *initial;

  // Parent of this GlobalEntity
  QatModule *parent;

  unsigned loads;

  unsigned stores;

  unsigned refers;

public:
  GlobalEntity(QatModule *_parent, std::string name, QatType *type,
               bool _is_variable, Value *_value,
               utils::VisibilityInfo _visibility);

  std::string getName() const;

  std::string getFullName() const;

  const utils::VisibilityInfo &getVisibility() const;

  bool hasInitial() const;

  unsigned getLoadCount() const;

  unsigned getStoreCount() const;

  unsigned getReferCount() const;
};

} // namespace qat::IR

#endif