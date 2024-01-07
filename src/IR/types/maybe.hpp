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
  useit bool     isTriviallyCopyable() const final { return subTy->isTriviallyCopyable(); }
  useit bool     isTriviallyMovable() const final { return subTy->isTriviallyMovable(); }
  useit bool     isTypePacked() const;

  useit bool isCopyConstructible() const final;
  void       copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  useit bool isMoveConstructible() const final;
  void       moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  useit bool isCopyAssignable() const final;
  void       copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  useit bool isMoveAssignable() const final;
  void       moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  useit bool isDestructible() const final;
  void       destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;

  useit inline bool canBePrerun() const final { return subTy->isTypeSized() && subTy->canBePrerun(); }
  useit inline bool canBePrerunGeneric() const final { return subTy->isTypeSized() && subTy->canBePrerunGeneric(); }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* value) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif