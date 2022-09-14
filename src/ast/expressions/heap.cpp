#include "./heap.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

#define MALLOC_ARG_BITWIDTH 64

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
      ctx->Error("The number of units to allocate should be of u64 type",
                 count->fileRange);
    } else {
      if (countRes->getType()->asUnsignedInteger()->getBitwidth() !=
          MALLOC_ARG_BITWIDTH) {
        ctx->Error("The number of units to allocate should be of u64 type",
                   count->fileRange);
      }
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

HeapGrow::HeapGrow(QatType *_type, Expression *_ptr, Expression *_count,
                   utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), ptr(_ptr), count(_count) {
}

IR::Value *HeapGrow::emit(IR::Context *ctx) {
  auto *typ    = type->emit(ctx);
  auto *ptrVal = ptr->emit(ctx);
  if (ptrVal->isImplicitPointer()) {
    if (ptrVal->getType()->isPointer()) {
      ptrVal->loadImplicitPointer(ctx->builder);
    } else {
      ctx->Error("The first argument to heap'grow should be a pointer to " +
                     ctx->highlightError(typ->toString()),
                 ptr->fileRange);
    }
  } else if (ptrVal->isReference()) {
    if (ptrVal->getType()->asReference()->getSubType()->isPointer()) {
      ptrVal = new IR::Value(
          ctx->builder.CreateLoad(
              ptrVal->getType()->asReference()->getSubType()->getLLVMType(),
              ptrVal->getLLVM()),
          ptrVal->getType()->asReference()->getSubType(), false,
          IR::Nature::temporary);
      if (!ptrVal->getType()->asPointer()->getSubType()->isSame(typ)) {
        ctx->Error("The first argument to heap'grow should be a pointer to " +
                       ctx->highlightError(typ->toString()),
                   ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument to heap'grow should be a pointer to " +
                     ctx->highlightError(typ->toString()),
                 ptr->fileRange);
    }
  }
  auto *countVal = count->emit(ctx);
  countVal->loadImplicitPointer(ctx->builder);
  if (countVal->isReference()) {
    countVal = new IR::Value(
        ctx->builder.CreateLoad(
            countVal->getType()->asReference()->getSubType()->getLLVMType(),
            countVal->getLLVM()),
        countVal->getType()->asReference()->getSubType(), false,
        IR::Nature::temporary);
  }
  if (countVal->getType()->isUnsignedInteger() &&
      (countVal->getType()->asUnsignedInteger()->getBitwidth() ==
       MALLOC_ARG_BITWIDTH)) {
    ctx->getMod()->linkNative(IR::NativeUnit::realloc);
    auto *reallocFn = ctx->getMod()->getLLVMModule()->getFunction("realloc");
    return new IR::Value(
        ctx->builder.CreatePointerCast(
            ctx->builder.CreateCall(
                reallocFn->getFunctionType(), reallocFn,
                {ctx->builder.CreatePointerCast(
                     ptrVal->getLLVM(),
                     llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
                 ctx->builder.CreateMul(
                     countVal->getLLVM(),
                     llvm::ConstantExpr::getSizeOf(
                         ptrVal->getType()->getLLVMType()))}),
            ptrVal->getType()->asPointer()->getLLVMType()),
        ptrVal->getType()->asPointer(), false, IR::Nature::temporary);
  } else {
    ctx->Error("The number of units to reallocate should of u64 type",
               count->fileRange);
  }
}

nuo::Json HeapGrow::toJson() const {
  return nuo::Json()
      ._("nodeType", "heapGrow")
      ._("type", type->toJson())
      ._("pointer", ptr->toJson())
      ._("count", count->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast