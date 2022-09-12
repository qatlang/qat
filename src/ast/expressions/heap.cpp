#include "./heap.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

HeapGet::HeapGet(ast::QatType *_type, ast::Expression *_count,
                 utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), count(_count) {}

IR::Value *HeapGet::emit(IR::Context *ctx) {
  auto      *mod      = ctx->getMod();
  IR::Value *countRes = nullptr;
  if (count) {
    countRes = count->emit(ctx);
    if (countRes->getType()->isReference()) {
      countRes = new IR::Value(
          ctx->builder.CreateLoad(
              countRes->getType()->asReference()->getSubType()->getLLVMType(),
              countRes->getLLVM()),
          countRes->getType()->asReference()->getSubType(), false,
          IR::Nature::temporary);
    }
    if (!countRes->getType()->isUnsignedInteger()) {
      ctx->Error(
          "The number of units to allocate should be of unsigned integer type",
          count->fileRange);
    }
  }
  // FIXME - Check for zero values
  llvm::Value *size   = nullptr;
  auto        *typRes = type->emit(ctx);
  if (typRes->isVoid()) {
    size = llvm::ConstantExpr::getSizeOf(llvm::Type::getInt8Ty(ctx->llctx));
  } else {
    size = llvm::ConstantExpr::getSizeOf(typRes->getLLVMType());
  }
  mod->linkNative(IR::NativeUnit::malloc);
  auto *resTy    = IR::PointerType::get(true, typRes, ctx->llctx);
  auto *mallocFn = mod->getLLVMModule()->getFunction("malloc");
  return new IR::Value(
      ctx->builder.CreatePointerCast(
          ctx->builder.CreateCall(
              mallocFn->getFunctionType(), mallocFn,
              {ctx->builder.CreateMul(
                  size,
                  (count ? countRes->getLLVM()
                         : llvm::ConstantInt::get(
                               llvm::Type::getInt64Ty(ctx->llctx), 1u)))}),
          resTy->getLLVMType()),
      resTy, false, IR::Nature::temporary);
}

nuo::Json HeapGet::toJson() const {
  return nuo::Json()
      ._("nodeType", "heapGet")
      ._("type", type->toJson())
      ._("count", count->toJson())
      ._("fileRange", fileRange);
}

HeapPut::HeapPut(Expression *pointer, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), ptr(pointer) {}

IR::Value *HeapPut::emit(IR::Context *ctx) {
  auto *exp = ptr->emit(ctx);
  if (exp->isImplicitPointer()) {
    exp->loadImplicitPointer(ctx->builder);
  }
  SHOW("Loaded implicit pointer")
  if (exp->getType()->isReference()) {
    exp = new IR::Value(
        ctx->builder.CreateLoad(
            exp->getType()->asReference()->getSubType()->getLLVMType(),
            exp->getLLVM()),
        exp->getType()->asReference()->getSubType(), false,
        IR::Nature::temporary);
  }
  SHOW("Loaded reference")
  if (exp->getType()->isPointer()) {
    auto *mod = ctx->getMod();
    mod->linkNative(IR::NativeUnit::free);
    auto *freeFn = mod->getLLVMModule()->getFunction("free");
    SHOW("Got free function")
    return new IR::Value(
        ctx->builder.CreateCall(
            freeFn->getFunctionType(), freeFn,
            {ctx->builder.CreatePointerCast(
                exp->getLLVM(),
                llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())}),
        IR::VoidType::get(ctx->llctx), false, IR::Nature::temporary);
  } else {
    ctx->Error("heap'put requires an expression that is of the pointer type",
               ptr->fileRange);
  }
}

nuo::Json HeapPut::toJson() const {
  return nuo::Json()
      ._("nodeType", "heapPut")
      ._("pointer", ptr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast