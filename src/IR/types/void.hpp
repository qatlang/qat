#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

class VoidType : public Type {
private:
  VoidType(llvm::LLVMContext& llctx);

public:
  useit static VoidType* get(llvm::LLVMContext& llctx);

  useit TypeKind type_kind() const override;
  useit String   to_string() const override;
};

} // namespace qat::ir

#endif