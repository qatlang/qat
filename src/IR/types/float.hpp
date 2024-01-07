#ifndef QAT_IR_TYPES_FLOAT_HPP
#define QAT_IR_TYPES_FLOAT_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

enum class FloatTypeKind { _brain, _16, _32, _64, _80, _128PPC, _128 };

class FloatType : public QatType {
private:
  FloatTypeKind kind;

  FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx);

public:
  useit static FloatType* get(FloatTypeKind _kind, llvm::LLVMContext& ctx);

  useit bool isTriviallyCopyable() const final { return true; }
  useit bool isTriviallyMovable() const final { return true; }

  useit bool          isTypeSized() const final;
  useit FloatTypeKind getKind() const;
  useit TypeKind      typeKind() const final;
  useit String        toString() const final;

  useit bool canBePrerun() const final { return true; }
  useit bool canBePrerunGeneric() const final { return true; }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif