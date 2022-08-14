#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../cli/color.hpp"
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
  IR::Function     *fn;         // Active function
  IR::CoreType     *activeType; // Active core type
  Vec<String>       exposed;
  bool              hasMain;

  // META

  Vec<fs::path> llvmOutputPaths;
  Vec<String>   nativeLibsToLink;

  useit QatModule *getMod() const; // Get the active IR module

  useit utils::RequesterInfo getReqInfo() const;

  static void Error(const String &message, const utils::FileRange &fileRange);
  static void Warning(const String &message, const utils::FileRange &fileRange);
  static String highlightError(const String &message,
                               const char   *color = colors::yellow);
  static String highlightWarning(const String &message,
                                 const char   *color = colors::yellow);
};

} // namespace qat::IR

#endif