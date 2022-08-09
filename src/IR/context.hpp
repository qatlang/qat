#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../utils/file_range.hpp"
#include "./qat_module.hpp"
#include "function.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <vector>

namespace qat::IR {

class Context {
private:
  using IRBuilderTy =
      llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>;

public:
  Context();

  llvm::LLVMContext llctx;
  IRBuilderTy       builder;
  QatModule        *mod;
  IR::Function     *fn;
  Vec<String>       exposed;
  bool              hasMain;

  // META

  Vec<fs::path> llvmOutputPaths;
  Vec<String>   nativeLibsToLink;

  useit QatModule *getMod() const; // Get the active IR module

  static void Error(const String &message, const utils::FileRange &fileRange);
  static void Warning(const String &message, const utils::FileRange &fileRange);
};

} // namespace qat::IR

#endif