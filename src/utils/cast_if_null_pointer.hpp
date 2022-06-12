#ifndef QAT_UTILS_CAST_IF_NULL_POINTER_HPP
#define QAT_UTILS_CAST_IF_NULL_POINTER_HPP

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace utils {

/**
 * @brief Cast a null pointer value to the type of the global variable
 *
 * @param gvar The global variable to store the pointer to
 * @param value The value to check and cast
 * @return llvm::Value*
 */
llvm::Value *cast_if_null_pointer(llvm::GlobalVariable *gvar,
                                  llvm::Value *value);

/**
 * @brief Cast a null pointer value to the type of the alloca
 *
 * @param gvar The alloca of the variable to store the pointer to
 * @param value The value to check and cast
 * @return llvm::Value*
 */
llvm::Value *cast_if_null_pointer(llvm::AllocaInst *alloca, llvm::Value *value);

} // namespace utils
} // namespace qat

#endif