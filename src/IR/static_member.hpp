#ifndef QAT_IR_STATIC_MEMBER_HPP
#define QAT_IR_STATIC_MEMBER_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"
#include "entity_overview.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class CoreType;
class QatModule;

class StaticMember final : public Value, public EntityOverview {
private:
  Identifier     name;
  CoreType*      parent;
  Value*         initial;
  u64            loads;
  u64            stores;
  u64            refers;
  VisibilityInfo visibility;

public:
  StaticMember(CoreType* _parent, Identifier name, QatType* _type, bool _is_variable, Value* _initial,
               const VisibilityInfo& _visibility);

  useit CoreType*             getParentType();
  useit Identifier            getName() const;
  useit String                getFullName() const;
  useit const VisibilityInfo& getVisibility() const;
  useit bool                  hasInitial() const;
  useit u64                   getLoadCount() const;
  useit u64                   getStoreCount() const;
  useit u64                   getReferCount() const;
  useit Json                  toJson() const;
  void                        updateOverview() final;

  ~StaticMember() final = default;
};

} // namespace qat::IR

#endif