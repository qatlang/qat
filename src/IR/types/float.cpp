#include "./float.hpp"
#include "../../memory_tracker.hpp"
#include "iostream"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

FloatType::FloatType(FloatTypeKind _kind, llvm::LLVMContext& llctx) : kind(_kind) {
  switch (kind) {
    case FloatTypeKind::_brain: {
      llvmType = llvm::Type::getBFloatTy(llctx);
      break;
    }
    case FloatTypeKind::_16: {
      llvmType = llvm::Type::getHalfTy(llctx);
      break;
    }
    case FloatTypeKind::_32: {
      llvmType = llvm::Type::getFloatTy(llctx);
      break;
    }
    case FloatTypeKind::_64: {
      llvmType = llvm::Type::getDoubleTy(llctx);
      break;
    }
    case FloatTypeKind::_80: {
      llvmType = llvm::Type::getX86_FP80Ty(llctx);
      break;
    }
    case FloatTypeKind::_128PPC: {
      llvmType = llvm::Type::getPPC_FP128Ty(llctx);
      break;
    }
    case FloatTypeKind::_128: {
      llvmType = llvm::Type::getFP128Ty(llctx);
      break;
    }
  }
  linkingName = "qat'" + toString();
}

FloatType* FloatType::get(FloatTypeKind _kind, llvm::LLVMContext& llctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isFloat()) {
      if (typ->asFloat()->getKind() == _kind) {
        return typ->asFloat();
      }
    }
  }
  return new FloatType(_kind, llctx);
}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

bool FloatType::isTypeSized() const { return true; }

String FloatType::toString() const {
  switch (kind) {
    case FloatTypeKind::_brain: {
      return "fbrain";
    }
    case FloatTypeKind::_16: {
      return "f16";
    }
    case FloatTypeKind::_32: {
      return "f32";
    }
    case FloatTypeKind::_64: {
      return "f64";
    }
    case FloatTypeKind::_80: {
      return "f80";
    }
    case FloatTypeKind::_128: {
      return "f128";
    }
    case FloatTypeKind::_128PPC: {
      return "f128ppc";
    }
  }
}

} // namespace qat::IR