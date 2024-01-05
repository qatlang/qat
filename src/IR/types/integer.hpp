#ifndef QAT_IR_TYPES_INTEGER_HPP
#define QAT_IR_TYPES_INTEGER_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class IntegerType : public QatType {
private:
  const u64    bitWidth;
  IR::Context* ctx;
  IntegerType(u64 _bitWidth, IR::Context* ctx);

public:
  useit static IntegerType* get(u64 _bits, IR::Context* ctx);
  useit bool                isBitWidth(u64 width) const;
  useit u64                 getBitwidth() const;
  useit TypeKind            typeKind() const final;
  useit String              toString() const final;
  useit bool                isTypeSized() const final;
  useit bool                isTriviallyCopyable() const final { return true; }
  useit bool                isTriviallyMovable() const final { return true; }

  useit bool canBePrerunGeneric() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif