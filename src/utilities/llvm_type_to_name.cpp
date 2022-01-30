#include "./llvm_type_to_name.hpp"

std::string qat::utilities::llvmTypeToName(llvm::Type *type) {
  if (type->isStructTy()) {
    return type->getStructName().str();
  } else {
    if (type->isIntegerTy()) {
      llvm::IntegerType *integerType = llvm::dyn_cast<llvm::IntegerType>(type);
      return "i" + std::to_string(integerType->getBitWidth());
    } else if (type->isPPC_FP128Ty()) {
      return "f128ppc";
    } else if (type->isFP128Ty()) {
      return "f128";
    } else if (type->isX86_FP80Ty()) {
      return "f80";
    } else if (type->isDoubleTy()) {
      return "f64";
    } else if (type->isFloatTy()) {
      return "f32";
    } else if (type->isVoidTy()) {
      return "void";
    } else if (type->isPointerTy()) {
      llvm::PointerType *pointerType = llvm::dyn_cast<llvm::PointerType>(type);
      if (pointerType->getElementType() != nullptr) {
        return llvmTypeToName(pointerType->getElementType()) + "#";
      } else {
        return "unknown#";
      }
    } else {
      return "unknown";
    }
  }
}