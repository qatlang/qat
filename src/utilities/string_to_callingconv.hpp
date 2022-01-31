#ifndef QAT_UTILITIES_STRING_TO_CALLINGCONV_HPP
#define QAT_UTILITIES_STRING_TO_CALLINGCONV_HPP

#include "llvm/IR/CallingConv.h"
#include <string>

namespace qat {
namespace utilities {
llvm::CallingConv::ID stringToCallingConv(std::string name);
}
} // namespace qat

#endif