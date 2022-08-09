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
  String                name;
  utils::VisibilityInfo visibility;
  Value                *initial;
  QatModule            *parent;
  u64                   loads;
  u64                   stores;
  u64                   refers;

public:
  GlobalEntity(QatModule *_parent, String name, QatType *type,
               bool _is_variable, Value *_value,
               utils::VisibilityInfo _visibility);

  useit String getName() const;
  useit String getFullName() const;
  useit const utils::VisibilityInfo &getVisibility() const;
  useit bool                         hasInitial() const;
  useit u64                          getLoadCount() const;
  useit u64                          getStoreCount() const;
  useit u64                          getReferCount() const;
  useit nuo::Json toJson() const;
};

} // namespace qat::IR

#endif