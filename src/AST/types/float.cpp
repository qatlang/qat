#include "./float.hpp"
namespace qat {
namespace AST {

FloatType::FloatType(const FloatTypeKind _kind, const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : kind(_kind), QatType(_variable, _filePlacement) {}

llvm::Type *FloatType::generate(IR::Generator *generator) {
  switch (kind) {
  case FloatTypeKind::_brain: {
    return generator->builder.getBFloatTy();
  }
  case FloatTypeKind::_half: {
    return generator->builder.getHalfTy();
  }
  case FloatTypeKind::_32: {
    return generator->builder.getFloatTy();
  }
  case FloatTypeKind::_64: {
    return generator->builder.getDoubleTy();
  }
  case FloatTypeKind::_80: {
    return llvm::Type::getX86_FP80Ty(generator->llvmContext);
  }
  case FloatTypeKind::_128PPC: {
    return llvm::Type::getPPC_FP128Ty(generator->llvmContext);
  }
  case FloatTypeKind::_128: {
    return llvm::Type::getFP128Ty(generator->llvmContext);
  }
  }
}

TypeKind FloatType::typeKind() { return TypeKind::Float; }

} // namespace AST
} // namespace qat