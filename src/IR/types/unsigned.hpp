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
  useit inline u64           getBitwidth() const { return bitWidth; }
  useit inline bool          isBitWidth(u64 width) const { return bitWidth == width; }
  useit inline bool          isBoolean() const { return isBool; }
  useit inline TypeKind      typeKind() const final { return TypeKind::unsignedInteger; }
  useit inline String        toString() const final { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

  useit inline bool isTypeSized() const final { return true; }
  useit inline bool isTriviallyCopyable() const final { return true; }
  useit inline bool isTriviallyMovable() const final { return true; }
  useit inline bool canBePrerun() const final { return true; }
  useit inline bool canBePrerunGeneric() const final { return true; }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif