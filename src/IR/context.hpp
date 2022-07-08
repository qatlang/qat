#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../utils/file_placement.hpp"
#include "./qat_module.hpp"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <vector>

namespace qat {
namespace IR {

class Context {
public:
  Context();

  // The IR module
  QatModule *mod;

  // The LLVMContext determines the context and scope of all instructions,
  // functions, blocks and variables
  llvm::LLVMContext llvmContext;

  // Provides Basic API to create instructions and then append
  // it to the end of a BasicBlock or to a specified location
  llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
      builder = llvm::IRBuilder(llvmContext, llvm::ConstantFolder(),
                                llvm::IRBuilderDefaultInserter(), nullptr,
                                llvm::None);

  // All the boxes exposed in the current scope. This will automatically
  // be populated and de-populated when the expose scope starts and ends
  std::vector<std::string> exposed;

  void throw_error(const std::string message,
                   const utils::FilePlacement placement);
};
} // namespace IR
} // namespace qat

#endif