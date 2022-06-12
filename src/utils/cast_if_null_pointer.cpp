#include "./cast_if_null_pointer.hpp"

namespace qat {
namespace utils {

llvm::Value *cast_if_null_pointer(llvm::GlobalVariable *gvar,
                                  llvm::Value *value) {
  if (gvar->getValueType()->isPointerTy()) {
    if (llvm::isa<llvm::Constant>(value)
            ? llvm::dyn_cast<llvm::Constant>(value)->getType()->isPointerTy()
            : false) {
      if (llvm::dyn_cast<llvm::Constant>(value)->isNullValue()) {
        return llvm::Constant::getNullValue(gvar->getValueType());
      }
    }
  }
  return value;
}

llvm::Value *cast_if_null_pointer(llvm::AllocaInst *alloca,
                                  llvm::Value *value) {
  if (alloca->getAllocatedType()->isPointerTy()) {
    if (llvm::isa<llvm::Constant>(value)
            ? llvm::dyn_cast<llvm::Constant>(value)->getType()->isPointerTy()
            : false) {
      if (llvm::dyn_cast<llvm::Constant>(value)->isNullValue()) {
        return llvm::Constant::getNullValue(alloca->getAllocatedType());
      }
    }
  }
  return value;
}

} // namespace utils
} // namespace qat