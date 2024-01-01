#ifndef QAT_IR_TYPES_REGION_HPP
#define QAT_IR_TYPES_REGION_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "llvm/IR/GlobalVariable.h"

namespace qat::IR {

class QatModule;

class Region : public QatType, public EntityOverview {
private:
  Identifier     name;
  QatModule*     parent;
  VisibilityInfo visibInfo;
  FileRange      fileRange;

  llvm::GlobalVariable* blocks;
  llvm::GlobalVariable* blockCount;
  llvm::Function*       ownFn;
  llvm::Function*       destructor;

  Region(Identifier _name, QatModule* _module, const VisibilityInfo& visibInfo, IR::Context* ctx, FileRange fileRange);

public:
  static Region* get(Identifier name, QatModule* parent, const VisibilityInfo& visibInfo, IR::Context* ctx,
                     FileRange fileRange);

  useit Identifier getName() const;
  useit String     getFullName() const;
  useit IR::QatModule* getParent() const;
  useit IR::Value* ownData(IR::QatType* _type, Maybe<llvm::Value*> count, IR::Context* ctx);
  void             destroyObjects(IR::Context* ctx);

  useit bool                  isAccessible(const AccessInfo& reqInfo) const;
  useit const VisibilityInfo& getVisibility() const;

  void updateOverview() final;

  useit TypeKind typeKind() const final { return TypeKind::region; }
  useit String   toString() const final;
};

} // namespace qat::IR

#endif