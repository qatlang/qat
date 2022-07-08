#include "./float.hpp"
#include "llvm/IR/Type.h"

namespace qat {
namespace IR {

FloatType::FloatType(llvm::LLVMContext &ctx, const FloatTypeKind _kind)
    : kind(_kind) {
  switch (kind) {
  case FloatTypeKind::_half: {
    llvmType = llvm::Type::getHalfTy(ctx);
  }
  case FloatTypeKind::_brain: {
    llvmType = llvm::Type::getBFloatTy(ctx);
  }
  case FloatTypeKind::_32: {
    llvmType = llvm::Type::getFloatTy(ctx);
  }
  case FloatTypeKind::_64: {
    llvmType = llvm::Type::getDoubleTy(ctx);
  }
  case FloatTypeKind::_80: {
    llvmType = llvm::Type::getX86_FP80Ty(ctx);
  }
  case FloatTypeKind::_128: {
    llvmType = llvm::Type::getFP128Ty(ctx);
  }
  case FloatTypeKind::_128PPC: {
    llvmType = llvm::Type::getPPC_FP128Ty(ctx);
  }
  }
}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

} // namespace IR
} // namespace qat