#ifndef QAT_IR_STATIC_MEMBER_HPP
#define QAT_IR_STATIC_MEMBER_HPP

#include "../utils/pointer_kind.hpp"
#include "../utils/variability.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include <optional>
#include <string>

namespace qat {
namespace IR {

class CoreType;
class QatModule;

class StaticMember : public Value {
private:
  std::string name;

  CoreType *parent;

  Value *initial;

  unsigned loads;

  unsigned stores;

  unsigned refers;

  utils::VisibilityInfo visibility;

public:
  StaticMember(CoreType *_parent, std::string name, QatType *_type,
               bool _is_variable, Value *_initial,
               utils::VisibilityInfo _visibility);

  CoreType *getParentType();

  std::string getName() const;

  std::string getFullName() const;

  const utils::VisibilityInfo &getVisibility() const;

  bool hasInitial() const;

  unsigned getLoadCount() const;

  unsigned getStoreCount() const;

  unsigned getReferCount() const;
};

} // namespace IR
} // namespace qat

#endif