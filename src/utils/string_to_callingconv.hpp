#ifndef QAT_UTILS_STRING_TO_CALLINGCONV_HPP
#define QAT_UTILS_STRING_TO_CALLINGCONV_HPP

#include "llvm/IR/CallingConv.h"
#include <string>

namespace qat {
namespace utils {
llvm::CallingConv::ID stringToCallingConv(std::string name);
}
} // namespace qat

#endif