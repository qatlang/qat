#include "./llvm_type_to_name.hpp"
namespace qat {
namespace utils {

std::string llvmTypeToName(llvm::Type *type) {
  if (type->isStructTy()) {
    auto structType = llvm::dyn_cast<llvm::StructType>(type);
    if (structType->isLiteral()) {
      std::string value = "(";
      for (std::size_t i = 0; i < structType->getNumContainedTypes(); i++) {
        value += llvmTypeToName(structType->getElementType(i));
        if ((i + 1 < structType->getNumContainedTypes()) ||
            (structType->getNumContainedTypes() == 1)) {
          value += "; ";
        }
      }
      value += ")";
      return value;
    } else {
      return structType->getName().str();
    }
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
      if (pointerType->getElementType()) {
        return "#[" + llvmTypeToName(pointerType->getElementType()) + "]";
      } else {
        return "unknown#";
      }
    } else {
      return "unknown";
    }
  }
}

} // namespace utils
} // namespace qat