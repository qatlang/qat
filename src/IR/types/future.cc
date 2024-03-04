#include "./future.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../logic.hpp"
#include "reference.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/AtomicOrdering.h"

namespace qat::ir {

FutureType::FutureType(Type* _subType, bool _isPacked, ir::Ctx* irCtx) : subTy(_subType), isPacked(_isPacked) {
  linkingName = "qat'future:[" + String(isPacked ? "pack," : "") + subTy->get_name_for_linking() + "]";
  llvmType    = llvm::StructType::create(irCtx->llctx,
                                         {
                                          llvm::Type::getInt64Ty(irCtx->llctx),
                                          llvm::Type::getInt64Ty(irCtx->llctx)->getPointerTo(),
                                      },
                                         linkingName, isPacked);
}

FutureType* FutureType::get(Type* subType, bool isPacked, ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->is_future()) {
      if (typ->as_future()->get_subtype()->is_same(subType)) {
        return typ->as_future();
      }
    }
  }
  return new FutureType(subType, isPacked, irCtx);
}

Type* FutureType::get_subtype() const { return subTy; }

String FutureType::to_string() const {
  return "future:[" + String(isPacked ? "pack, " : "") + subTy->to_string() + "]";
}

TypeKind FutureType::type_kind() const { return TypeKind::future; }

bool FutureType::is_destructible() const { return true; }

bool FutureType::is_type_sized() const { return true; }

bool FutureType::is_type_packed() const { return isPacked; }

bool FutureType::is_copy_constructible() const { return true; }

bool FutureType::is_copy_assignable() const { return true; }

void FutureType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  irCtx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Add,
      irCtx->builder.CreateLoad(
          llvm::Type::getInt64PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()),
          irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
}

void FutureType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  irCtx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Add,
      irCtx->builder.CreateLoad(
          llvm::Type::getInt64PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()),
          irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  destroy_value(irCtx, first, fun);
  irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
}

void FutureType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
  auto* currBlock = fun->get_block();
  auto* selfVal   = instance->get_llvm();
  irCtx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Sub,
      irCtx->builder.CreateLoad(
          llvm::Type::getInt64PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()),
          irCtx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  SHOW("Creating zero comparison")
  auto* zeroCmp = irCtx->builder.CreateICmpEQ(
      irCtx->builder.CreateLoad(
          llvm::Type::getInt64Ty(irCtx->llctx),
          irCtx->builder.CreateLoad(
              llvm::Type::getInt64PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()),
              irCtx->builder.CreateStructGEP(llvmType, selfVal, 1u))),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u));
  auto* trueBlock = new ir::Block(fun, currBlock);
  auto* restBlock = new ir::Block(fun, currBlock->getParent());
  restBlock->link_previous_block(currBlock);
  irCtx->builder.CreateCondBr(zeroCmp, trueBlock->get_bb(), restBlock->get_bb());
  trueBlock->set_active(irCtx->builder);
  fun->get_module()->link_native(NativeUnit::free);
  auto* freeFn = fun->get_module()->get_llvm_module()->getFunction("free");
  if (subTy->is_destructible()) {
    subTy->destroy_value(
        irCtx,
        ir::Value::get(
            irCtx->builder.CreatePointerCast(
                irCtx->builder.CreateInBoundsGEP(
                    llvm::Type::getInt8Ty(irCtx->llctx),
                    irCtx->builder.CreatePointerCast(
                        irCtx->builder.CreateInBoundsGEP(
                            llvm::Type::getInt64Ty(irCtx->llctx),
                            irCtx->builder.CreateLoad(
                                llvm::Type::getInt64PtrTy(irCtx->llctx,
                                                          irCtx->dataLayout.value().getProgramAddressSpace()),
                                irCtx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
                            {llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u)}),
                        llvm::Type::getInt8PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace())),
                    {llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u)}),
                subTy->get_llvm_type()->getPointerTo(irCtx->dataLayout.value().getProgramAddressSpace())),
            ir::ReferenceType::get(false, subTy, irCtx), false),
        fun);
  }
  irCtx->builder.CreateCall(
      freeFn->getFunctionType(), freeFn,
      {irCtx->builder.CreatePointerCast(
          irCtx->builder.CreateLoad(
              llvm::Type::getInt64PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()),
              irCtx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
          llvm::Type::getInt8PtrTy(irCtx->llctx, irCtx->dataLayout.value().getProgramAddressSpace()))});
  (void)ir::add_branch(irCtx->builder, restBlock->get_bb());
  restBlock->set_active(irCtx->builder);
}

} // namespace qat::ir