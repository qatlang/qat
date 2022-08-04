#ifndef QAT_IR_LLVM_HELPER_HPP
#define QAT_IR_LLVM_HELPER_HPP

#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class llvmHelper {
public:
  llvmHelper() : builder(llctx) {}

  llvm::LLVMContext                                                     llctx;
  llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> builder;
};

} // namespace qat::IR

#endif