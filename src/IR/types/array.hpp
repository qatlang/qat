#ifndef QAT_IR_TYPES_ARRAY_HPP
#define QAT_IR_TYPES_ARRAY_HPP

#include "../../utils/helpers.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

/**
 *  A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType : public QatType {
private:
  QatType* elementType;
  u64      length;

  ArrayType(QatType* _element_type, u64 _length, llvm::LLVMContext& llctx);

public:
  useit static ArrayType* get(QatType* elementType, u64 length, llvm::LLVMContext& llctx);

  useit QatType* getElementType();
  useit u64      getLength() const;
  useit TypeKind typeKind() const final;
  useit String   toString() const final;

  useit bool canBeConstGeneric() const final;
  useit Maybe<String> toConstGenericString(IR::ConstantValue* val) const final;
  useit Maybe<bool> equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const final;
  useit bool        isTypeSized() const final;

  useit bool isDestructible() const final;
  void       destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
};

} // namespace qat::IR

#endif