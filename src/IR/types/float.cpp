#include "./float.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

FloatType::FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx) : kind(_kind) {
  switch (kind) {
    case FloatTypeKind::_brain: {
      llvmType = llvm::Type::getBFloatTy(ctx);
    }
    case FloatTypeKind::_half: {
      llvmType = llvm::Type::getHalfTy(ctx);
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
    case FloatTypeKind::_128PPC: {
      llvmType = llvm::Type::getPPC_FP128Ty(ctx);
    }
    case FloatTypeKind::_128: {
      llvmType = llvm::Type::getFP128Ty(ctx);
    }
  }
}

FloatType* FloatType::get(FloatTypeKind _kind, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isFloat()) {
      if (typ->asFloat()->getKind() == _kind) {
        return typ->asFloat();
      }
    }
  }
  return new FloatType(_kind, ctx);
}

TypeKind FloatType::typeKind() const { return TypeKind::Float; }

FloatTypeKind FloatType::getKind() const { return kind; }

String FloatType::toString() const {
  switch (kind) {
    case FloatTypeKind::_half: {
      return "fhalf";
    }
    case FloatTypeKind::_brain: {
      return "fbrain";
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

Json FloatType::toJson() const {
  return Json()._("id", getID())._("typeKind", "float")._("floatKind", toString().substr(1));
}

} // namespace qat::IR