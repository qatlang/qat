#ifndef QAT_UTILS_LLVMTYPE_TO_NAME_HPP
#define QAT_UTILS_LLVMTYPE_TO_NAME_HPP

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace utils {
std::string llvmTypeToName(llvm::Type *type);
}
} // namespace qat
#endif