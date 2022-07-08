#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"
#include <llvm/IR/LLVMContext.h>

namespace qat {
namespace IR {

class VoidType : public QatType {
public:
  VoidType(llvm::LLVMContext &llctx);

  TypeKind typeKind() const;
};

} // namespace IR
} // namespace qat

#endif