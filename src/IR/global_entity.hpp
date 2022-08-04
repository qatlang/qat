#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/visibility.hpp"
#include "./definable.hpp"
#include "./value.hpp"

#include <optional>
#include <string>

namespace qat::IR {

class QatModule;

class GlobalEntity : public Value, public Definable {
private:
  // Name of this GlobalEntity
  String name;

  // Details about the visibility of this global
  utils::VisibilityInfo visibility;

  Value *initial;

  // Parent of this GlobalEntity
  QatModule *parent;

  u64 loads;

  u64 stores;

  u64 refers;

public:
  GlobalEntity(QatModule *_parent, String name, QatType *type,
               bool _is_variable, Value *_value,
               utils::VisibilityInfo _visibility);

  String getName() const;

  String getFullName() const;

  const utils::VisibilityInfo &getVisibility() const;

  bool hasInitial() const;

  u64 getLoadCount() const;

  u64 getStoreCount() const;

  u64 getReferCount() const;

  void defineLLVM(llvmHelper &help) const;

  void defineCPP(cpp::File &file) const;

  nuo::Json toJson() const;
};

} // namespace qat::IR

#endif