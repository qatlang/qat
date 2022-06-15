#ifndef QAT_UTILS_POINTER_KIND_HPP
#define QAT_UTILS_POINTER_KIND_HPP

#include "llvm/IR/Argument.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

namespace qat {
namespace utils {

class PointerKind {
public:
  static void set(llvm::LLVMContext &ctx, llvm::AllocaInst *inst,
                  bool is_reference);

  static void set(llvm::LLVMContext &ctx, llvm::GlobalVariable *gvar,
                  bool is_reference);

  static void set(llvm::Argument *arg, bool is_reference);

  static bool is_reference(llvm::AllocaInst *inst);

  static bool is_reference(llvm::GlobalVariable *gvar);

  static bool is_reference(llvm::Argument *arg);
};

} // namespace utils
} // namespace qat

#endif