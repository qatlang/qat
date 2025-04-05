#include "./char.hpp"

#include <llvm/IR/Type.h>

namespace qat::ir {

CharType::CharType(llvm::LLVMContext& llctx) {
	llvmType    = llvm::cast<llvm::Type>(llvm::Type::getIntNTy(llctx, 21u));
	linkingName = "qat'char";
}

} // namespace qat::ir
