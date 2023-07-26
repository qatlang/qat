#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

// Unsigned integer datatype in the language
class UnsignedType : public QatType {
private:
  u64  bitWidth;
  bool isBool;

  UnsignedType(u64 _bitWidth, llvm::LLVMContext& ctx, bool _isBool = false);

public:
  useit static UnsignedType* get(u64 bits, llvm::LLVMContext& ctx);
  useit static UnsignedType* getBool(llvm::LLVMContext& ctx);
  useit u64                  getBitwidth() const;
  useit bool                 isBitWidth(u64 width) const;
  useit bool                 isBoolean() const;
  useit TypeKind             typeKind() const final;
  useit String               toString() const final;

  useit bool canBeConstGeneric() const final;
  useit Maybe<String> toConstGenericString(IR::ConstantValue* val) const final;
  useit Maybe<bool> equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const final;
};

} // namespace qat::IR

#endif