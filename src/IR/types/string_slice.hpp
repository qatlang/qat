#ifndef QAT_IR_TYPES_STRING_SLICE_HPP
#define QAT_IR_TYPES_STRING_SLICE_HPP

#include "qat_type.hpp"

namespace qat::IR {

class StringSliceType : public QatType {
private:
  StringSliceType(llvm::LLVMContext& llctx);

public:
  useit static StringSliceType* get(llvm::LLVMContext& llctx);
  useit TypeKind                typeKind() const override;
  useit String                  toString() const override;

  useit bool canBePrerunGeneric() const final;
  useit bool isTypeSized() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif