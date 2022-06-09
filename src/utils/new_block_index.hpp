#ifndef QAT_UTILS_NEW_BLOCK_INDEX_HPP
#define QAT_UTILS_NEW_BLOCK_INDEX_HPP

#include "llvm/IR/Function.h"

namespace qat {
namespace utils {

/**
 * @brief Get the index for a new BasicBlock to be inserted into the specified
 * function
 *
 * @param fn Function to get the name from
 * @return unsigned
 */
unsigned new_block_index(llvm::Function *fn);

} // namespace utils
} // namespace qat

#endif