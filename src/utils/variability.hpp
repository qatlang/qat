#ifndef QAT_UTILS_VARIABILITY_HPP
#define QAT_UTILS_VARIABILITY_HPP

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"

namespace qat {
namespace utils {
/**
 *  Variability is a static class that hosts the functions
 * associated with the metadata that handles variability of values
 *
 */
class Variability {
public:
  static bool get(llvm::Instruction *value);
  static bool get(llvm::Argument *value);
  static bool get(llvm::GlobalVariable *value);
  static void set(llvm::LLVMContext &context, llvm::Instruction *value,
                  bool is_var);
  static void set(llvm::Argument *value, bool is_var);
  static void set(llvm::LLVMContext &ctx, llvm::Function *fn, bool is_var);
  static void propagate(llvm::LLVMContext &context, llvm::Instruction *source,
                        llvm::Instruction *target);
  static void propagate(llvm::Argument *source, llvm::Argument *target);
  static void propagate(llvm::Instruction *source, llvm::Argument *target);
  static void propagate(llvm::LLVMContext &context, llvm::Argument *source,
                        llvm::Instruction *target);
  static void propagate(llvm::LLVMContext &context,
                        llvm::GlobalVariable *source,
                        llvm::Instruction *target);
};
} // namespace utils
} // namespace qat

#endif