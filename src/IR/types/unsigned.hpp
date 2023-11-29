#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

// Unsigned integer datatype in the language
class UnsignedType : public QatType {
private:
  IR::Context* ctx;
  u64          bitWidth;
  bool         isBool;

  UnsignedType(u64 _bitWidth, IR::Context* ctx, bool _isBool = false);

public:
  useit static UnsignedType* get(u64 bits, IR::Context* llctx);
  useit static UnsignedType* getBool(IR::Context* llctx);
  useit u64                  getBitwidth() const;
  useit bool                 isBitWidth(u64 width) const;
  useit bool                 isBoolean() const;
  useit TypeKind             typeKind() const final;
  useit String               toString() const final;

  useit bool isTypeSized() const final;
  useit bool isTriviallyCopyable() const final { return true; }
  useit bool isTriviallyMovable() const final { return true; }
  useit bool canBePrerunGeneric() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif