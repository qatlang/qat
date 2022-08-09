#ifndef QAT_IR_STATIC_MEMBER_HPP
#define QAT_IR_STATIC_MEMBER_HPP

#include "../utils/visibility.hpp"
#include "./value.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class CoreType;
class QatModule;

class StaticMember : public Value {
private:
  String name;

  CoreType *parent;

  Value *initial;

  u64 loads;

  u64 stores;

  u64 refers;

  utils::VisibilityInfo visibility;

public:
  StaticMember(CoreType *_parent, String name, QatType *_type,
               bool _is_variable, Value *_initial,
               const utils::VisibilityInfo &_visibility);

  useit CoreType *getParentType();
  useit String    getName() const;
  useit String    getFullName() const;
  useit const utils::VisibilityInfo &getVisibility() const;
  useit bool                         hasInitial() const;
  useit u64                          getLoadCount() const;
  useit u64                          getStoreCount() const;
  useit u64                          getReferCount() const;
  useit nuo::Json toJson() const;
};

} // namespace qat::IR

#endif