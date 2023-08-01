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
    if (count->nodeType() == NodeType::unsignedLiteral) {
      ((UnsignedLiteral*)count)->setType(IR::UnsignedType::get(64u, ctx->llctx)); // NOLINT(readability-magic-numbers)
    } else if (count->nodeType() == NodeType::integerLiteral) {
      ((IntegerLiteral*)count)->setType(IR::UnsignedType::get(64u, ctx->llctx)); // NOLINT(readability-magic-numbers)
    } else if (count->nodeType() == NodeType::Default) {
      ctx->Error("Default value for u64 is 0, which is an invalid value for the number of instances to allocate",
                 count->fileRange);
    }
    countRes = count->emit(ctx);
    if (countRes->getType()->isReference()) {
      countRes->loadImplicitPointer(ctx->builder);
      countRes = new IR::Value(
          ctx->builder.CreateLoad(countRes->getType()->asReference()->getSubType()->getLLVMType(), countRes->getLLVM()),
          countRes->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
    }
    if (!countRes->getType()->isUnsignedInteger()) {
      ctx->Error("The number of instances to allocate should be of u64 type", count->fileRange);
    } else {
      if (countRes->getType()->asUnsignedInteger()->getBitwidth() != MALLOC_ARG_BITWIDTH) {
        ctx->Error("The number of instances to allocate should be of u64 type", count->fileRange);
      }
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
  auto* resTy    = IR::PointerType::get(true, typRes, IR::PointerOwner::OfHeap(), count != nullptr, ctx);
  auto* mallocFn = mod->getLLVMModule()->getFunction("malloc");
  if (resTy->isMulti()) {
    SHOW("Creating alloca for multi pointer")
    auto* llAlloca = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), resTy->getLLVMType());
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
  auto* exp = ptr->emit(ctx);
  SHOW("Loaded implicit pointer")
  if (exp->getType()->isReference() && exp->getType()->asReference()->getSubType()->isPointer()) {
    exp->loadImplicitPointer(ctx->builder);
    if (!exp->getType()->asReference()->getSubType()->asPointer()->isMulti()) {
      exp = new IR::Value(
          ctx->builder.CreateLoad(exp->getType()->asReference()->getSubType()->getLLVMType(), exp->getLLVM()),
          exp->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
    }
  } else if (exp->isImplicitPointer() && exp->getType()->isPointer() && !exp->getType()->asPointer()->isMulti()) {
    exp->loadImplicitPointer(ctx->builder);
  }
  SHOW("Loaded reference")
  if (exp->getType()->isPointer() ||
      (exp->getType()->isReference() && exp->getType()->asReference()->getSubType()->isPointer() &&
       exp->getType()->asReference()->getSubType()->asPointer()->isMulti())) {
    auto* expPtrTy =
        exp->isReference() ? exp->getType()->asReference()->getSubType()->asPointer() : exp->getType()->asPointer();
    if (!expPtrTy->getOwner().isHeap()) {
      ctx->Error("The ownership of this pointer is " +
                     ctx->highlightError(exp->getType()->asPointer()->getOwner().toString()) +
                     " and hence cannot be used in heap'put",
                 fileRange);
    }
    auto* mod       = ctx->getMod();
    auto* currBlock = ctx->fn->getBlock();
    auto* trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
    auto* restBlock = new IR::Block(ctx->fn, nullptr);
    restBlock->linkPrevBlock(currBlock);
    SHOW("Created condition blocks")
    SHOW("Exp type is: " << exp->getType()->toString())
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpNE(
            ctx->builder.CreatePtrDiff(
                expPtrTy->getSubType()->getLLVMType(),
                exp->getType()->asPointer()->isMulti()
                    ? ctx->builder.CreateLoad(
                          llvm::PointerType::get(expPtrTy->getSubType()->getLLVMType(),
                                                 ctx->dataLayout->getProgramAddressSpace()),
                          ctx->builder.CreateStructGEP(exp->isReference()
                                                           ? exp->getType()->asReference()->getSubType()->getLLVMType()
                                                           : exp->getType()->getLLVMType(),
                                                       exp->getLLVM(), 0u))
                    : ((exp->isReference() || exp->isImplicitPointer())
                           ? ctx->builder.CreateLoad(exp->isReference()
                                                         ? exp->getType()->asReference()->getSubType()->getLLVMType()
                                                         : exp->getType()->getLLVMType(),
                                                     exp->getLLVM())
                           : exp->getLLVM()),
                llvm::ConstantPointerNull::get(llvm::PointerType::get(expPtrTy->getSubType()->getLLVMType(),
                                                                      ctx->dataLayout->getProgramAddressSpace()))),
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
        trueBlock->getBB(), restBlock->getBB());
    mod->linkNative(IR::NativeUnit::free);
    auto* freeFn = mod->getLLVMModule()->getFunction("free");
    SHOW("Got free function")
    trueBlock->setActive(ctx->builder);
    ctx->builder.CreateCall(
        freeFn->getFunctionType(), freeFn,
        {ctx->builder.CreatePointerCast(
            expPtrTy->isMulti()
                ? ctx->builder.CreateLoad(llvm::PointerType::get(expPtrTy->getSubType()->getLLVMType(),
                                                                 ctx->dataLayout->getProgramAddressSpace()),
                                          ctx->builder.CreateStructGEP(expPtrTy->getLLVMType(), exp->getLLVM(), 0u))
                : exp->getLLVM(),
            llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
    return nullptr;
  } else {
    ctx->Error("heap'put requires an expression that is of the pointer type, but the provided expression is of type " +
                   exp->getType()->toString(),
               ptr->fileRange);
  }
  return nullptr;
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
        ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->highlightError(typ->toString()),
                   ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->highlightError(typ->toString()),
                 ptr->fileRange);
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
    ptrVal->makeImplicitPointer(ctx, None);
  }
  auto* countVal = count->emit(ctx);
  countVal->loadImplicitPointer(ctx->builder);
  if (countVal->isReference()) {
    countVal = new IR::Value(
        ctx->builder.CreateLoad(countVal->getType()->asReference()->getSubType()->getLLVMType(), countVal->getLLVM()),
        countVal->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
  }
  if (countVal->getType()->isUnsignedInteger() &&
      (countVal->getType()->asUnsignedInteger()->getBitwidth() == MALLOC_ARG_BITWIDTH)) {
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
                ctx->builder.CreateMul(countVal->getLLVM(),
                                       llvm::ConstantExpr::getSizeOf(ptrVal->getType()->getLLVMType()))}),
        llvm::PointerType::get(ptrType->asPointer()->getSubType()->getLLVMType(),
                                  ctx->dataLayout->getProgramAddressSpace()));
    auto* resAlloc = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), ptrType->getLLVMType());
    SHOW("Storing raw pointer into multipointer")
    ctx->builder.CreateStore(ptrRes, ctx->builder.CreateStructGEP(ptrType->getLLVMType(), resAlloc, 0u));
    SHOW("Storing count into multipointer")
    ctx->builder.CreateStore(countVal->getLLVM(), ctx->builder.CreateStructGEP(ptrType->getLLVMType(), resAlloc, 1u));
    return new IR::Value(ptrVal->getLLVM(), ptrVal->getType(), false, IR::Nature::temporary);
  } else {
    ctx->Error("The number of units to reallocate should of u64 type", count->fileRange);
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