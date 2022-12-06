#ifndef QAT_IR_TYPES_REGION_HPP
#define QAT_IR_TYPES_REGION_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "llvm/IR/GlobalVariable.h"

namespace qat::IR {

class QatModule;

class Region : public QatType {
private:
  String                name;
  QatModule*            parent;
  utils::VisibilityInfo visibInfo;

  llvm::GlobalVariable* blocks;
  llvm::GlobalVariable* blockCount;
  llvm::Function*       ownFn;
  llvm::Function*       destructor;

  Region(String _name, QatModule* _module, const utils::VisibilityInfo& visibInfo, IR::Context* ctx);

public:
  static Region* get(String name, QatModule* parent, const utils::VisibilityInfo& visibInfo, IR::Context* ctx);

  useit String getName() const;
  useit String getFullName() const;
  useit IR::Value* ownData(IR::QatType* _type, Maybe<llvm::Value*> count, IR::Context* ctx);
  void             destroyObjects(IR::Context* ctx);

  useit bool  isAccessible(const utils::RequesterInfo& reqInfo) const;
  useit const utils::VisibilityInfo& getVisibility() const;

  useit TypeKind typeKind() const final { return TypeKind::region; }
  useit String   toString() const final;
  useit Json     toJson() const final { return {}; }
};

} // namespace qat::IR

#endif