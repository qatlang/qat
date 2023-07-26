#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class VoidType : public QatType {
private:
  VoidType(llvm::LLVMContext& ctx);

public:
  useit static VoidType* get(llvm::LLVMContext& ctx);

  useit TypeKind typeKind() const override;
  useit String   toString() const override;
};

} // namespace qat::IR

#endif