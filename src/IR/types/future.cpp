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

namespace qat::IR {

FutureType::FutureType(QatType* _subType, bool _isPacked, IR::Context* ctx) : subTy(_subType), isPacked(_isPacked) {
  linkingName = "qat'future:[" + String(isPacked ? "pack," : "") + subTy->getNameForLinking() + "]";
  llvmType    = llvm::StructType::create(ctx->llctx,
                                         {
                                          llvm::Type::getInt64Ty(ctx->llctx),
                                          llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
                                      },
                                         linkingName, isPacked);
}

FutureType* FutureType::get(QatType* subType, bool isPacked, IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isFuture()) {
      if (typ->asFuture()->getSubType()->isSame(subType)) {
        return typ->asFuture();
      }
    }
  }
  return new FutureType(subType, isPacked, ctx);
}

QatType* FutureType::getSubType() const { return subTy; }

String FutureType::toString() const { return "future:[" + String(isPacked ? "pack, " : "") + subTy->toString() + "]"; }

TypeKind FutureType::typeKind() const { return TypeKind::future; }

bool FutureType::isDestructible() const { return true; }

bool FutureType::isTypeSized() const { return true; }

bool FutureType::isTypePacked() const { return isPacked; }

bool FutureType::isCopyConstructible() const { return true; }

bool FutureType::isCopyAssignable() const { return true; }

void FutureType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  ctx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Add,
      ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                              ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
}

void FutureType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  ctx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Add,
      ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                              ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  destroyValue(ctx, first, fun);
  ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
}

void FutureType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  auto* currBlock = fun->getBlock();
  auto* selfVal   = instance->getLLVM();
  ctx->builder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Sub,
      ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                              ctx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u), llvm::MaybeAlign(0u),
      llvm::AtomicOrdering::AcquireRelease);
  SHOW("Creating zero comparison")
  auto* zeroCmp = ctx->builder.CreateICmpEQ(
      ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                              ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(
                                                          ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                                                      ctx->builder.CreateStructGEP(llvmType, selfVal, 1u))),
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u));
  auto* trueBlock = new IR::Block(fun, currBlock);
  auto* restBlock = new IR::Block(fun, currBlock->getParent());
  restBlock->linkPrevBlock(currBlock);
  ctx->builder.CreateCondBr(zeroCmp, trueBlock->getBB(), restBlock->getBB());
  trueBlock->setActive(ctx->builder);
  ctx->getMod()->linkNative(NativeUnit::free);
  auto* freeFn = ctx->getMod()->getLLVMModule()->getFunction("free");
  if (subTy->isDestructible()) {
    subTy->destroyValue(
        ctx,
        new IR::Value(
            ctx->builder.CreatePointerCast(
                ctx->builder.CreateInBoundsGEP(
                    llvm::Type::getInt8Ty(ctx->llctx),
                    ctx->builder.CreatePointerCast(
                        ctx->builder.CreateInBoundsGEP(
                            llvm::Type::getInt64Ty(ctx->llctx),
                            ctx->builder.CreateLoad(
                                llvm::Type::getInt64PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                                ctx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
                            {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)}),
                        llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace())),
                    {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)}),
                subTy->getLLVMType()->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace())),
            IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
        fun);
  }
  ctx->builder.CreateCall(freeFn->getFunctionType(), freeFn,
                          {ctx->builder.CreatePointerCast(
                              ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(
                                                          ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
                                                      ctx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
                              llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()))});
  (void)IR::addBranch(ctx->builder, restBlock->getBB());
  restBlock->setActive(ctx->builder);
}

} // namespace qat::IR