#ifndef QAT_IR_MAYBE_HPP
#define QAT_IR_MAYBE_HPP

#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class MaybeType : public QatType {
private:
  QatType* subTy;
  bool     isPacked;

  MaybeType(QatType* subTy, bool isPacked, IR::Context* ctx);

public:
  useit static MaybeType* get(QatType* subTy, bool isPacked, IR::Context* ctx);

  useit bool     hasSizedSubType(IR::Context* ctx) const;
  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final { return TypeKind::maybe; }
  useit bool     isTypeSized() const final;
  useit bool     isTypePacked() const;
  useit bool     isDestructible() const final;
  void           destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
};

} // namespace qat::IR

#endif