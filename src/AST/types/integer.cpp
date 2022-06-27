#include "./integer.hpp"

namespace qat {
namespace AST {

IntegerType::IntegerType(const unsigned int _bitWidth, const bool _isUnsigned,
                         const bool _variable,
                         const utils::FilePlacement _filePlacement)
    : bitWidth(_bitWidth), isUnsigned(_isUnsigned),
      QatType(_variable, _filePlacement) {}

llvm::Type *IntegerType::generate(qat::IR::Generator *generator) {
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

bool IntegerType::isBitWidth(const unsigned int width) const {
  return bitWidth == width;
}

bool IntegerType::isUnsignedType() const noexcept { return isUnsigned; }

TypeKind IntegerType::typeKind() { return TypeKind::integer; }

backend::JSON IntegerType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "integer")
      ._("bitWidth", bitWidth)
      ._("isUnsigned", isUnsigned)
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat