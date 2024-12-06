#ifndef QAT_UTILS_STRING_TO_CALLINGCONV_HPP
#define QAT_UTILS_STRING_TO_CALLINGCONV_HPP

#include "./helpers.hpp"
#include <llvm/IR/CallingConv.h>

namespace qat::utils {

llvm::CallingConv::ID stringToCallingConv(const String& name);

} // namespace qat::utils

#endif
