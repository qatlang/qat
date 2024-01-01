#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"
#include "entity_overview.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class QatModule;

class GlobalEntity : public Value, public EntityOverview {
private:
  Identifier             name;
  VisibilityInfo         visibility;
  QatModule*             parent;
  Maybe<llvm::Constant*> initialValue;

  u64 loads;
  u64 stores;
  u64 refers;

public:
  GlobalEntity(QatModule* _parent, Identifier _name, QatType* _type, bool _is_variable,
               Maybe<llvm::Constant*> initialValue, llvm::Value* _value, const VisibilityInfo& _visibility);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit IR::QatModule* getParent() const;
  useit bool           hasInitialValue() const;
  useit llvm::Constant*       getInitialValue() const;
  useit const VisibilityInfo& getVisibility() const;
  useit u64                   getLoadCount() const;
  useit u64                   getStoreCount() const;
  useit u64                   getReferCount() const;
};

} // namespace qat::IR

#endif