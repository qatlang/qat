#include "./unsigned.hpp"

namespace qat {
namespace AST {

UnsignedType::UnsignedType(const unsigned int _bitWidth, const bool _variable,
                           const utils::FilePlacement _filePlacement)
    : bitWidth(_bitWidth), QatType(_variable, _filePlacement) {}

llvm::Type *UnsignedType::generate(qat::IR::Generator *generator) {
  switch (bitWidth) {
  case 1: {
    return llvm::Type::getInt1Ty(generator->llvmContext);
  }
  case 8: {
    return llvm::Type::getInt8Ty(generator->llvmContext);
  }
  case 16: {
    return llvm::Type::getInt16Ty(generator->llvmContext);
  }
  case 32: {
    return llvm::Type::getInt32Ty(generator->llvmContext);
  }
  case 64: {
    return llvm::Type::getInt64Ty(generator->llvmContext);
  }
  case 128: {
    return llvm::Type::getInt128Ty(generator->llvmContext);
  }
  default: {
    return llvm::Type::getIntNTy(generator->llvmContext, bitWidth);
  }
  }
}

bool UnsignedType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() { return TypeKind::unsignedInteger; }

backend::JSON UnsignedType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "unsignedInteger")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat