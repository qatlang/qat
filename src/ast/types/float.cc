#include "./float.hpp"
#include "../../IR/types/float.hpp"

namespace qat::ast {

Maybe<usize> FloatType::getTypeSizeInBits(EmitCtx* ctx) const {
  switch (kind) {
    case ir::FloatTypeKind::_32:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getFloatTy(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_64:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getDoubleTy(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_80:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getX86_FP80Ty(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_128:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getFP128Ty(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_128PPC:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getPPC_FP128Ty(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_16:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getHalfTy(ctx->irCtx->llctx));
    case ir::FloatTypeKind::_brain:
      return ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::Type::getBFloatTy(ctx->irCtx->llctx));
  }
}

ir::Type* FloatType::emit(EmitCtx* ctx) { return ir::FloatType::get(kind, ctx->irCtx->llctx); }

String FloatType::kindToString(ir::FloatTypeKind kind) {
  switch (kind) {
    case ir::FloatTypeKind::_16: {
      return "f16";
    }
    case ir::FloatTypeKind::_brain: {
      return "fbrain";
    }
    case ir::FloatTypeKind::_32: {
      return "f32";
    }
    case ir::FloatTypeKind::_64: {
      return "f64";
    }
    case ir::FloatTypeKind::_80: {
      return "f80";
    }
    case ir::FloatTypeKind::_128: {
      return "f128";
    }
    case ir::FloatTypeKind::_128PPC: {
      return "f128ppc";
    }
  }
}

AstTypeKind FloatType::type_kind() const { return AstTypeKind::FLOAT; }

Json FloatType::to_json() const {
  return Json()._("typeKind", "float")._("floatTypeKind", kindToString(kind))._("fileRange", fileRange);
}

String FloatType::to_string() const { return kindToString(kind); }

} // namespace qat::ast
