#include "./void.hpp"

#include <llvm/IR/Type.h>

namespace qat::ir {

VoidType* VoidType::voidType = nullptr;

VoidType::VoidType(llvm::LLVMContext& llctx) {
	llvmType    = llvm::Type::getVoidTy(llctx);
	linkingName = "qat'void";
	voidType    = this;
}

} // namespace qat::ir
