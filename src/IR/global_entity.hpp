#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"
#include "entity_overview.hpp"

namespace qat::ir {

class Mod;

class PrerunGlobal : public PrerunValue, public EntityOverview {
  Identifier     name;
  VisibilityInfo visibility;
  Mod*           parent;

public:
  PrerunGlobal(Mod* _parent, Identifier _name, Type* _type, llvm::Constant* _constant, VisibilityInfo _visibility,
               FileRange _fileRange);

  useit Identifier get_name() const { return name; }
  useit String     get_full_name() const;
  useit ir::Mod*              getParent() const { return parent; }
  useit VisibilityInfo const& get_visibility() const { return visibility; }
};

class GlobalEntity : public Value, public EntityOverview {
  Identifier             name;
  VisibilityInfo         visibility;
  Mod*                   parent;
  Maybe<llvm::Constant*> initialValue;

public:
  GlobalEntity(Mod* _parent, Identifier _name, Type* _type, bool _is_variable, Maybe<llvm::Constant*> initialValue,
               llvm::Value* _value, const VisibilityInfo& _visibility);

  useit static GlobalEntity* get(Mod* _parent, Identifier _name, Type* _type, bool _is_variable,
                                 Maybe<llvm::Constant*> initialValue, llvm::Value* _value,
                                 const VisibilityInfo& _visibility) {
    return new GlobalEntity(_parent, _name, _type, _is_variable, initialValue, _value, _visibility);
  }

  useit Identifier get_name() const;
  useit String     get_full_name() const;
  useit ir::Mod* get_module() const;
  useit bool     hasInitialValue() const;
  useit llvm::Constant*       getInitialValue() const;
  useit const VisibilityInfo& get_visibility() const;
};

} // namespace qat::ir

#endif
