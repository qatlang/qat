#include "./heap.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

#define MALLOC_ARG_BITWIDTH 64

HeapGet::HeapGet(ast::QatType* _type, ast::Expression* _count, FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), count(_count) {}

IR::Value* HeapGet::emit(IR::Context* ctx) {
  auto*      mod      = ctx->getMod();
  IR::Value* countRes = nullptr;
  if (count) {
    if (count->nodeType() == NodeType::Default) {
      ctx->Error("Default value for usize is 0, which is an invalid value for the number of instances to allocate",
                 count->fileRange);
    } else if (count->hasTypeInferrance()) {
      count->asTypeInferrable()->setInferenceType(IR::CType::getUsize(ctx));
    }
    countRes = count->emit(ctx);
    countRes->loadImplicitPointer(ctx->builder);
    if (countRes->getType()->isReference()) {
      countRes = new IR::Value(
          ctx->builder.CreateLoad(countRes->getType()->asReference()->getSubType()->getLLVMType(), countRes->getLLVM()),
          countRes->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
    }
    if (!countRes->getType()->isCType() || !countRes->getType()->asCType()->isUsize()) {
      ctx->Error("The number of instances to allocate should be of " + IR::CType::getUsize(ctx)->toString() + " type",
                 count->fileRange);
    }
  }
  // FIXME - Check for zero values
  llvm::Value* size   = nullptr;
  auto*        typRes = type->emit(ctx);
  if (typRes->isVoid()) {
    size = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(ctx->llctx),
        mod->getLLVMModule()->getDataLayout().getTypeAllocSize(llvm::Type::getInt8Ty(ctx->llctx)));
  } else {
    size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                  mod->getLLVMModule()->getDataLayout().getTypeAllocSize(typRes->getLLVMType()));
  }
  mod->linkNative(IR::NativeUnit::malloc);
  auto* resTy    = IR::PointerType::get(true, typRes, false, IR::PointerOwner::OfHeap(), count != nullptr, ctx);
  auto* mallocFn = mod->getLLVMModule()->getFunction("malloc");
  if (resTy->isMulti()) {
    SHOW("Creating alloca for multi pointer")
    auto* llAlloca = IR::Logic::newAlloca(ctx->getActiveFunction(), None, resTy->getLLVMType());
    ctx->builder.CreateStore(
        ctx->builder.CreatePointerCast(
            ctx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn,
                                    {count ? ctx->builder.CreateMul(size, countRes->getLLVM()) : size}),
            llvm::PointerType::get(resTy->getSubType()->getLLVMType(), ctx->dataLayout->getProgramAddressSpace())),
        ctx->builder.CreateStructGEP(resTy->getLLVMType(), llAlloca, 0u));
    SHOW("Storing the size of the multi pointer")
    ctx->builder.CreateStore(countRes->getLLVM(), ctx->builder.CreateStructGEP(resTy->getLLVMType(), llAlloca, 1u));
    return new IR::Value(llAlloca, resTy, false, IR::Nature::temporary);
  } else {
    return new IR::Value(
        ctx->builder.CreatePointerCast(
            ctx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn,
                                    {count ? ctx->builder.CreateMul(size, countRes->getLLVM()) : size}),
            resTy->getLLVMType()),
        resTy, false, IR::Nature::temporary);
  }
}

Json HeapGet::toJson() const {
  return Json()
      ._("nodeType", "heapGet")
      ._("type", type->toJson())
      ._("count", count->toJson())
      ._("fileRange", fileRange);
}

HeapPut::HeapPut(Expression* pointer, FileRange _fileRange) : Expression(std::move(_fileRange)), ptr(pointer) {}

IR::Value* HeapPut::emit(IR::Context* ctx) {
  if (ptr->nodeType() == NodeType::nullPointer) {
    ctx->Error("Null pointer cannot be freed", ptr->fileRange);
  }
  auto* exp   = ptr->emit(ctx);
  auto  expTy = exp->isReference() ? exp->getType()->asReference()->getSubType() : exp->getType();
  if (expTy->isPointer()) {
    auto ptrTy = expTy->asPointer();
    if (!ptrTy->getOwner().isHeap()) {
      ctx->Error("The pointer type of this expression is " + ctx->highlightError(ptrTy->toString()) +
                     " which does not have heap ownership and hence cannot be used here",
                 ptr->fileRange);
    }
  } else {
    ctx->Error(
        "Expecting an expression having a pointer type with heap ownership. The provided expression is of type " +
            ctx->highlightError(expTy->toString()) + ". Other expression types are not supported",
        ptr->fileRange);
  }
  llvm::Value* candExp = nullptr;
  if (exp->getType()->isReference() || exp->isImplicitPointer()) {
    if (exp->getType()->isReference()) {
      exp->loadImplicitPointer(ctx->builder);
    }
    exp = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), exp->getLLVM()), expTy, false,
                        IR::Nature::temporary);
  }
  // FIXME - CONSIDER PRERUN POINTERS
  candExp   = expTy->asPointer()->isMulti() ? ctx->builder.CreateExtractValue(exp->getLLVM(), {0u}) : exp->getLLVM();
  auto* mod = ctx->getMod();
  mod->linkNative(IR::NativeUnit::free);
  auto* freeFn = mod->getLLVMModule()->getFunction("free");
  ctx->builder.CreateCall(freeFn->getFunctionType(), freeFn,
                          {ctx->builder.CreatePointerCast(candExp, llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
  return new IR::Value(nullptr, IR::VoidType::get(ctx->llctx), false, IR::Nature::temporary);
}

Json HeapPut::toJson() const {
  return Json()._("nodeType", "heapPut")._("pointer", ptr->toJson())._("fileRange", fileRange);
}

HeapGrow::HeapGrow(QatType* _type, Expression* _ptr, Expression* _count, FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), ptr(_ptr), count(_count) {}

IR::Value* HeapGrow::emit(IR::Context* ctx) {
  auto* typ    = type->emit(ctx);
  auto* ptrVal = ptr->emit(ctx);
  // FIXME - Add check to see if the new size is lower than the previous size
  IR::PointerType* ptrType = nullptr;
  if (ptrVal->isReference()) {
    if (!ptrVal->getType()->asReference()->isSubtypeVariable()) {
      ctx->Error("This reference does not have variability and hence the pointer inside cannot be grown",
                 ptr->fileRange);
    }
    if (ptrVal->getType()->asReference()->getSubType()->isPointer()) {
      ptrType = ptrVal->getType()->asReference()->getSubType()->asPointer();
      ptrVal->loadImplicitPointer(ctx->builder);
      if (!ptrType->getOwner().isHeap()) {
        ctx->Error("The ownership of this pointer is not " + ctx->highlightError("heap") +
                       " and hence cannot be used in heap'grow",
                   fileRange);
      }
      if (!ptrType->isMulti()) {
        ctx->Error("The type of the expression is " +
                       ctx->highlightError(ptrVal->getType()->asReference()->getSubType()->toString()) +
                       " which is not a multi pointer and hence cannot be grown",
                   ptr->fileRange);
      }
      if (!ptrType->getSubType()->isSame(typ)) {
        ctx->Error("The first argument should be a pointer to " + ctx->highlightError(typ->toString()), ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument should be a pointer to " + ctx->highlightError(typ->toString()), ptr->fileRange);
    }
  } else if (ptrVal->isImplicitPointer()) {
    if (ptrVal->getType()->isPointer()) {
      if (ptrVal->isVariable()) {
        ctx->Error("This expression is not a variable", fileRange);
      }
      ptrType = ptrVal->getType()->asPointer();
      if (!ptrType->getOwner().isHeap()) {
        ctx->Error("The ownership of this pointer is " + ctx->highlightError("heap") +
                       " and hence cannot be used in heap'grow",
                   fileRange);
      }
      if (!ptrType->isMulti()) {
        ctx->Error("The type of the expression is " + ctx->highlightError(ptrVal->getType()->toString()) +
                       " which is not a multi pointer and hence cannot be grown",
                   ptr->fileRange);
      }
      if (!ptrType->getSubType()->isSame(typ)) {
        ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->highlightError(typ->toString()),
                   ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->highlightError(typ->toString()),
                 ptr->fileRange);
    }
  } else {
    ptrType = ptrVal->getType()->asPointer();
    if (!ptrType->getOwner().isHeap()) {
      ctx->Error("Expected a multipointer with " + ctx->highlightError("heap") +
                     " ownership. The ownership of this pointer is " +
                     ctx->highlightError(ptrType->getOwner().toString()) + " and hence cannot be used.",
                 fileRange);
    }
    if (!ptrType->isMulti()) {
      ctx->Error("The type of the expression is " + ctx->highlightError(ptrVal->getType()->toString()) +
                     " which is not a multi pointer and hence cannot be used here",
                 ptr->fileRange);
    }
    if (!ptrType->getSubType()->isSame(typ)) {
      ctx->Error("The first argument should be a pointer to " + ctx->highlightError(typ->toString()), ptr->fileRange);
    }
    ptrVal->makeImplicitPointer(ctx, None);
  }
  auto* countVal = count->emit(ctx);
  countVal->loadImplicitPointer(ctx->builder);
  if (countVal->isReference()) {
    countVal = new IR::Value(
        ctx->builder.CreateLoad(countVal->getType()->asReference()->getSubType()->getLLVMType(), countVal->getLLVM()),
        countVal->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
  }
  if (countVal->getType()->isCType() && countVal->getType()->asCType()->isUsize()) {
    ctx->getMod()->linkNative(IR::NativeUnit::realloc);
    auto* reallocFn = ctx->getMod()->getLLVMModule()->getFunction("realloc");
    auto* ptrRes    = ctx->builder.CreatePointerCast(
        ctx->builder.CreateCall(
            reallocFn->getFunctionType(), reallocFn,
            {ctx->builder.CreatePointerCast(
                 ctx->builder.CreateLoad(
                     llvm::PointerType::get(typ->getLLVMType(), ctx->dataLayout->getProgramAddressSpace()),
                     ctx->builder.CreateStructGEP(ptrType->getLLVMType(), ptrVal->getLLVM(), 0u)),
                 llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
                ctx->builder.CreateMul(
                 countVal->getLLVM(),
                 llvm::ConstantInt::get(IR::CType::getUsize(ctx)->getLLVMType(),
                                           ctx->dataLayout.value().getTypeStoreSize(ptrVal->getType()->getLLVMType())))}),
        llvm::PointerType::get(ptrType->asPointer()->getSubType()->getLLVMType(),
                                  ctx->dataLayout->getProgramAddressSpace()));
    auto* resAlloc = IR::Logic::newAlloca(ctx->getActiveFunction(), None, ptrType->getLLVMType());
    SHOW("Storing raw pointer into multipointer")
    ctx->builder.CreateStore(ptrRes, ctx->builder.CreateStructGEP(ptrType->getLLVMType(), resAlloc, 0u));
    SHOW("Storing count into multipointer")
    ctx->builder.CreateStore(countVal->getLLVM(), ctx->builder.CreateStructGEP(ptrType->getLLVMType(), resAlloc, 1u));
    return new IR::Value(ptrVal->getLLVM(), ptrVal->getType(), false, IR::Nature::temporary);
  } else {
    ctx->Error("The number of units to reallocate should be of " +
                   ctx->highlightError(IR::CType::getUsize(ctx)->toString()) + " type",
               count->fileRange);
  }
}

Json HeapGrow::toJson() const {
  return Json()
      ._("nodeType", "heapGrow")
      ._("type", type->toJson())
      ._("pointer", ptr->toJson())
      ._("count", count->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast