#ifndef QAT_UTILITIES_STRING_TO_CALLINGCONV_HPP
#define QAT_UTILITIES_STRING_TO_CALLINGCONV_HPP

#include <string>
#include "llvm/IR/CallingConv.h"

namespace qat {
    namespace utilities {
        llvm::CallingConv::ID stringToCallingConv(std::string name);
    }
}

#endif