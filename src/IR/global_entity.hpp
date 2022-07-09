#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

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

class QatModule;

class GlobalEntity : public Value {
private:
  // Name of this GlobalEntity
  std::string name;

  // Details about the visibility of this global
  utils::VisibilityInfo visibility;

  // Parent of this GlobalEntity
  QatModule *parent;

  unsigned loads;

  unsigned stores;

  unsigned refers;

public:
  GlobalEntity(QatModule *_parent, std::string name, QatType *type,
               bool _is_variable, bool _is_reference, Value *_value,
               utils::VisibilityInfo _visibility);

  std::string getName() const;

  std::string getFullName() const;

  const utils::VisibilityInfo &getVisibility() const;

  unsigned getLoadCount() const;

  unsigned getStoreCount() const;

  unsigned getReferCount() const;

  // llvm::Value *
  // get(llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
  //         &builder,
  //     const bool as_reference, const utils::RequesterInfo &req_info);
};

} // namespace IR
} // namespace qat

#endif