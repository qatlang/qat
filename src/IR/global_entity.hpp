#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class QatModule;

class GlobalEntity : public Value {
private:
  Identifier            name;
  utils::VisibilityInfo visibility;
  QatModule*            parent;
  u64                   loads;
  u64                   stores;
  u64                   refers;

public:
  GlobalEntity(QatModule* _parent, Identifier _name, QatType* _type, bool _is_variable, llvm::Value* _value,
               const utils::VisibilityInfo& _visibility);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit const utils::VisibilityInfo& getVisibility() const;
  useit u64                          getLoadCount() const;
  useit u64                          getStoreCount() const;
  useit u64                          getReferCount() const;
  useit Json                         toJson() const;
};

} // namespace qat::IR

#endif