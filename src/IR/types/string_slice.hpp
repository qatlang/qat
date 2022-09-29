#ifndef QAT_IR_TYPES_STRING_SLICE_HPP
#define QAT_IR_TYPES_STRING_SLICE_HPP

#include "qat_type.hpp"

namespace qat::IR {

class StringSliceType : public QatType {
private:
  StringSliceType(llvm::LLVMContext &ctx);

public:
  useit static StringSliceType *get(llvm::LLVMContext &ctx);
  useit TypeKind                typeKind() const override;
  useit String                  toString() const override;
  useit Json                    toJson() const override;
};

} // namespace qat::IR

#endif