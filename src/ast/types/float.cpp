#include "./float.hpp"
#include "../../IR/types/float.hpp"

namespace qat::ast {

Maybe<usize> FloatType::getTypeSizeInBits(IR::Context* ctx) const {
  switch (kind) {
    case IR::FloatTypeKind::_32:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::Type::getFloatTy(ctx->llctx));
    case IR::FloatTypeKind::_64:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getDoubleTy(ctx->llctx));
    case IR::FloatTypeKind::_80:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getX86_FP80Ty(ctx->llctx));
    case IR::FloatTypeKind::_128:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::Type::getFP128Ty(ctx->llctx));
    case IR::FloatTypeKind::_128PPC:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getPPC_FP128Ty(ctx->llctx));
    case IR::FloatTypeKind::_16:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::Type::getHalfTy(ctx->llctx));
    case IR::FloatTypeKind::_brain:
      return ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getBFloatTy(ctx->llctx));
  }
}

IR::QatType* FloatType::emit(IR::Context* ctx) { return IR::FloatType::get(kind, ctx->llctx); }

String FloatType::kindToString(IR::FloatTypeKind kind) {
  switch (kind) {
    case IR::FloatTypeKind::_16: {
      return "f16";
    }
    case IR::FloatTypeKind::_brain: {
      return "fbrain";
    }
    case IR::FloatTypeKind::_32: {
      return "f32";
    }
    case IR::FloatTypeKind::_64: {
      return "f64";
    }
    case IR::FloatTypeKind::_80: {
      return "f80";
    }
    case IR::FloatTypeKind::_128: {
      return "f128";
    }
    case IR::FloatTypeKind::_128PPC: {
      return "f128ppc";
    }
  }
}

AstTypeKind FloatType::typeKind() const { return AstTypeKind::FLOAT; }

Json FloatType::toJson() const {
  return Json()._("typeKind", "float")._("floatTypeKind", kindToString(kind))._("fileRange", fileRange);
}

String FloatType::toString() const { return kindToString(kind); }

} // namespace qat::ast