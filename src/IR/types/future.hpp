#ifndef QAT_IR_FUTURE_HPP
#define QAT_IR_FUTURE_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::IR {

class FutureType : public QatType {
private:
  QatType* subTy;
  bool     isPacked;

  FutureType(QatType* subType, bool isPacked, IR::Context* ctx);

public:
  useit static FutureType* get(QatType* subType, bool isPacked, IR::Context* ctx);

  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final;
  useit bool     isTypeSized() const final;
  useit bool     isTypePacked() const;

  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final { return false; }
  useit bool isMoveAssignable() const final { return false; }
  void       copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void       copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  //   void       moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  //   void       moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  useit bool isDestructible() const final;
  void       destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;
};

} // namespace qat::IR

#endif