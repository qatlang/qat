#ifndef QAT_IR_MAYBE_HPP
#define QAT_IR_MAYBE_HPP

#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class MaybeType : public QatType {
private:
  QatType* subTy;

  MaybeType(QatType* subTy, llvm::LLVMContext& ctx);

public:
  useit static MaybeType* get(QatType* subTy, llvm::LLVMContext& ctx);

  useit bool     hasSizedSubType() const;
  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final { return TypeKind::maybe; }
  useit bool     isDestructible() const final;
  void           destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
};

} // namespace qat::IR

#endif