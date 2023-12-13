#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

#include <vector>

namespace qat::IR {

/**
 *  Tuples are product types. It is a defined fixed-length sequence of
 * other types
 *
 */
class TupleType : public QatType {
private:
  Vec<QatType*> subTypes;
  bool          isPacked;

  TupleType(Vec<QatType*> _types, bool _isPacked, llvm::LLVMContext& llctx);

public:
  useit static TupleType* get(Vec<QatType*> types, bool isPacked, llvm::LLVMContext& llctx);

  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final;
  useit bool isMoveAssignable() const final;
  useit bool isDestructible() const final;

  void copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) final;
  void destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) final;

  useit bool isTypeSized() const;
  useit bool isTriviallyCopyable() const final {
    for (auto* sub : subTypes) {
      if (!sub->isTriviallyCopyable()) {
        return false;
      }
    }
    return true;
  }
  useit bool isTriviallyMovable() const final {
    for (auto* sub : subTypes) {
      if (!sub->isTriviallyMovable()) {
        return false;
      }
    }
    return true;
  }

  useit Vec<QatType*> getSubTypes() const;
  useit QatType*      getSubtypeAt(u64 index);
  useit u64           getSubTypeCount() const;
  useit bool          isPackedTuple() const;
  useit TypeKind      typeKind() const final;
  useit String        toString() const final;
};

} // namespace qat::IR

#endif