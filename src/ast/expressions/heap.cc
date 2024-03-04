#include "./heap.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

#define MALLOC_ARG_BITWIDTH 64

ir::Value* HeapGet::emit(EmitCtx* ctx) {
  auto       mod      = ctx->mod;
  ir::Value* countRes = nullptr;
  if (count) {
    if (count->nodeType() == NodeType::DEFAULT) {
      ctx->Error("Default value for usize is 0, which is an invalid value for the number of instances to allocate",
                 count->fileRange);
    } else if (count->hasTypeInferrance()) {
      count->asTypeInferrable()->setInferenceType(ir::CType::get_usize(ctx->irCtx));
    }
    countRes = count->emit(ctx);
    countRes->load_ghost_pointer(ctx->irCtx->builder);
    if (countRes->get_ir_type()->is_reference()) {
      countRes = ir::Value::get(
          ctx->irCtx->builder.CreateLoad(countRes->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
                                         countRes->get_llvm()),
          countRes->get_ir_type()->as_reference()->get_subtype(), false);
    }
    if (!countRes->get_ir_type()->is_ctype() || !countRes->get_ir_type()->as_ctype()->is_usize()) {
      ctx->Error("The number of instances to allocate should be of " + ir::CType::get_usize(ctx->irCtx)->to_string() +
                     " type",
                 count->fileRange);
    }
  }
  // FIXME - Check for zero values
  llvm::Value* size   = nullptr;
  auto*        typRes = type->emit(ctx);
  if (typRes->is_void()) {
    size = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(ctx->irCtx->llctx),
        mod->get_llvm_module()->getDataLayout().getTypeAllocSize(llvm::Type::getInt8Ty(ctx->irCtx->llctx)));
  } else {
    size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                                  mod->get_llvm_module()->getDataLayout().getTypeAllocSize(typRes->get_llvm_type()));
  }
  mod->link_native(ir::NativeUnit::malloc);
  auto* resTy    = ir::PointerType::get(true, typRes, false, ir::PointerOwner::OfHeap(), count != nullptr, ctx->irCtx);
  auto* mallocFn = mod->get_llvm_module()->getFunction("malloc");
  if (resTy->isMulti()) {
    SHOW("Creating alloca for multi pointer")
    auto* llAlloca = ir::Logic::newAlloca(ctx->get_fn(), None, resTy->get_llvm_type());
    ctx->irCtx->builder.CreateStore(
        ctx->irCtx->builder.CreatePointerCast(
            ctx->irCtx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn,
                                           {count ? ctx->irCtx->builder.CreateMul(size, countRes->get_llvm()) : size}),
            llvm::PointerType::get(resTy->get_subtype()->get_llvm_type(),
                                   ctx->irCtx->dataLayout->getProgramAddressSpace())),
        ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), llAlloca, 0u));
    SHOW("Storing the size of the multi pointer")
    ctx->irCtx->builder.CreateStore(countRes->get_llvm(),
                                    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), llAlloca, 1u));
    return ir::Value::get(llAlloca, resTy, false);
  } else {
    return ir::Value::get(
        ctx->irCtx->builder.CreatePointerCast(
            ctx->irCtx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn,
                                           {count ? ctx->irCtx->builder.CreateMul(size, countRes->get_llvm()) : size}),
            resTy->get_llvm_type()),
        resTy, false);
  }
}

Json HeapGet::to_json() const {
  return Json()
      ._("nodeType", "heapGet")
      ._("type", type->to_json())
      ._("count", count->to_json())
      ._("fileRange", fileRange);
}

ir::Value* HeapPut::emit(EmitCtx* ctx) {
  if (ptr->nodeType() == NodeType::NULL_POINTER) {
    ctx->Error("Null pointer cannot be freed", ptr->fileRange);
  }
  auto* exp   = ptr->emit(ctx);
  auto  expTy = exp->is_reference() ? exp->get_ir_type()->as_reference()->get_subtype() : exp->get_ir_type();
  if (expTy->is_pointer()) {
    auto ptrTy = expTy->as_pointer();
    if (!ptrTy->getOwner().isHeap()) {
      ctx->Error("The pointer type of this expression is " + ctx->color(ptrTy->to_string()) +
                     " which does not have heap ownership and hence cannot be used here",
                 ptr->fileRange);
    }
  } else {
    ctx->Error(
        "Expecting an expression having a pointer type with heap ownership. The provided expression is of type " +
            ctx->color(expTy->to_string()) + ". Other expression types are not supported",
        ptr->fileRange);
  }
  llvm::Value* candExp = nullptr;
  if (exp->get_ir_type()->is_reference() || exp->is_ghost_pointer()) {
    if (exp->get_ir_type()->is_reference()) {
      exp->load_ghost_pointer(ctx->irCtx->builder);
    }
    exp = ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), exp->get_llvm()), expTy, false);
  }
  // FIXME - CONSIDER PRERUN POINTERS
  candExp =
      expTy->as_pointer()->isMulti() ? ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {0u}) : exp->get_llvm();
  auto* mod = ctx->mod;
  mod->link_native(ir::NativeUnit::free);
  auto* freeFn = mod->get_llvm_module()->getFunction("free");
  ctx->irCtx->builder.CreateCall(
      freeFn->getFunctionType(), freeFn,
      {ctx->irCtx->builder.CreatePointerCast(candExp, llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo())});
  return ir::Value::get(nullptr, ir::VoidType::get(ctx->irCtx->llctx), false);
}

Json HeapPut::to_json() const {
  return Json()._("nodeType", "heapPut")._("pointer", ptr->to_json())._("fileRange", fileRange);
}

ir::Value* HeapGrow::emit(EmitCtx* ctx) {
  auto* typ    = type->emit(ctx);
  auto* ptrVal = ptr->emit(ctx);
  // FIXME - Add check to see if the new size is lower than the previous size
  ir::PointerType* ptrType = nullptr;
  if (ptrVal->is_reference()) {
    if (!ptrVal->get_ir_type()->as_reference()->isSubtypeVariable()) {
      ctx->Error("This reference does not have variability and hence the pointer inside cannot be grown",
                 ptr->fileRange);
    }
    if (ptrVal->get_ir_type()->as_reference()->get_subtype()->is_pointer()) {
      ptrType = ptrVal->get_ir_type()->as_reference()->get_subtype()->as_pointer();
      ptrVal->load_ghost_pointer(ctx->irCtx->builder);
      if (!ptrType->getOwner().isHeap()) {
        ctx->Error("The ownership of this pointer is not " + ctx->color("heap") +
                       " and hence cannot be used in heap:grow",
                   fileRange);
      }
      if (!ptrType->isMulti()) {
        ctx->Error("The type of the expression is " +
                       ctx->color(ptrVal->get_ir_type()->as_reference()->get_subtype()->to_string()) +
                       " which is not a multi pointer and hence cannot be grown",
                   ptr->fileRange);
      }
      if (!ptrType->get_subtype()->is_same(typ)) {
        ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
    }
  } else if (ptrVal->is_ghost_pointer()) {
    if (ptrVal->get_ir_type()->is_pointer()) {
      if (ptrVal->is_variable()) {
        ctx->Error("This expression is not a variable", fileRange);
      }
      ptrType = ptrVal->get_ir_type()->as_pointer();
      if (!ptrType->getOwner().isHeap()) {
        ctx->Error("The ownership of this pointer is not " + ctx->color("heap") +
                       " and hence cannot be used in heap:grow",
                   fileRange);
      }
      if (!ptrType->isMulti()) {
        ctx->Error("The type of the expression is " + ctx->color(ptrVal->get_ir_type()->to_string()) +
                       " which is not a multi pointer and hence cannot be grown",
                   ptr->fileRange);
      }
      if (!ptrType->get_subtype()->is_same(typ)) {
        ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->color(typ->to_string()),
                   ptr->fileRange);
      }
    } else {
      ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->color(typ->to_string()),
                 ptr->fileRange);
    }
  } else {
    ptrType = ptrVal->get_ir_type()->as_pointer();
    if (!ptrType->getOwner().isHeap()) {
      ctx->Error("Expected a multipointer with " + ctx->color("heap") +
                     " ownership. The ownership of this pointer is " + ctx->color(ptrType->getOwner().to_string()) +
                     " and hence cannot be used.",
                 fileRange);
    }
    if (!ptrType->isMulti()) {
      ctx->Error("The type of the expression is " + ctx->color(ptrVal->get_ir_type()->to_string()) +
                     " which is not a multi pointer and hence cannot be used here",
                 ptr->fileRange);
    }
    if (!ptrType->get_subtype()->is_same(typ)) {
      ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
    }
  }
  auto* countVal = count->emit(ctx);
  countVal->load_ghost_pointer(ctx->irCtx->builder);
  if (countVal->is_reference()) {
    countVal = ir::Value::get(
        ctx->irCtx->builder.CreateLoad(countVal->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
                                       countVal->get_llvm()),
        countVal->get_ir_type()->as_reference()->get_subtype(), false);
  }
  if (countVal->get_ir_type()->is_ctype() && countVal->get_ir_type()->as_ctype()->is_usize()) {
    ctx->mod->link_native(ir::NativeUnit::realloc);
    auto* reallocFn = ctx->mod->get_llvm_module()->getFunction("realloc");
    auto* ptrRes    = ctx->irCtx->builder.CreatePointerCast(
        ctx->irCtx->builder.CreateCall(
            reallocFn->getFunctionType(), reallocFn,
            {ctx->irCtx->builder.CreatePointerCast(
                 ctx->irCtx->builder.CreateLoad(
                     llvm::PointerType::get(ptrType->get_subtype()->get_llvm_type(),
                                               ctx->irCtx->dataLayout->getProgramAddressSpace()),
                     ptrVal->is_value()
                            ? ctx->irCtx->builder.CreateExtractValue(ptrVal->get_llvm(), {0u})
                            : ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), ptrVal->get_llvm(), 0u)),
                 llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo()),
                ctx->irCtx->builder.CreateMul(countVal->get_llvm(),
                                              llvm::ConstantInt::get(ir::CType::get_usize(ctx->irCtx)->get_llvm_type(),
                                                                     ctx->irCtx->dataLayout.value().getTypeStoreSize(
                                                                      ptrType->get_subtype()->get_llvm_type())))}),
        llvm::PointerType::get(ptrType->as_pointer()->get_subtype()->get_llvm_type(),
                                  ctx->irCtx->dataLayout->getProgramAddressSpace()));
    auto* resAlloc = ir::Logic::newAlloca(ctx->get_fn(), None, ptrType->get_llvm_type());
    SHOW("Storing raw pointer into multipointer")
    ctx->irCtx->builder.CreateStore(ptrRes,
                                    ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), resAlloc, 0u));
    SHOW("Storing count into multipointer")
    ctx->irCtx->builder.CreateStore(countVal->get_llvm(),
                                    ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), resAlloc, 1u));
    return ir::Value::get(resAlloc, ptrType, false);
  } else {
    ctx->Error("The number of units to reallocate should be of " +
                   ctx->color(ir::CType::get_usize(ctx->irCtx)->to_string()) + " type",
               count->fileRange);
  }
}

Json HeapGrow::to_json() const {
  return Json()
      ._("nodeType", "heapGrow")
      ._("type", type->to_json())
      ._("pointer", ptr->to_json())
      ._("count", count->to_json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast