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

class PrerunGlobal : public PrerunValue, public EntityOverview {
  Identifier     name;
  VisibilityInfo visibility;
  QatModule*     parent;

public:
  PrerunGlobal(QatModule* _parent, Identifier _name, QatType* _type, llvm::Constant* _constant,
               VisibilityInfo _visibility, FileRange _fileRange);

  useit inline Identifier getName() const { return name; }
  useit String            getFullName() const;
  useit IR::QatModule*        getParent() const { return parent; }
  useit VisibilityInfo const& getVisibility() const { return visibility; }
};

class GlobalEntity : public Value, public EntityOverview {
  Identifier             name;
  VisibilityInfo         visibility;
  QatModule*             parent;
  Maybe<llvm::Constant*> initialValue;

public:
  GlobalEntity(QatModule* _parent, Identifier _name, QatType* _type, bool _is_variable,
               Maybe<llvm::Constant*> initialValue, llvm::Value* _value, const VisibilityInfo& _visibility);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit IR::QatModule* getParent() const;
  useit bool           hasInitialValue() const;
  useit llvm::Constant*       getInitialValue() const;
  useit const VisibilityInfo& getVisibility() const;
};

} // namespace qat::IR

#endif