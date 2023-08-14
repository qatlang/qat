#include "./future.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../logic.hpp"
#include "reference.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/AtomicOrdering.h"

namespace qat::IR {

FutureType::FutureType(QatType* _subType, IR::Context* ctx) : subTy(_subType) {
  if (subTy->isVoid()) {
    llvmType = llvm::StructType::create(ctx->llctx,
                                        {llvm::Type::getInt64Ty(ctx->llctx),
                                         llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
                                         llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo()},
                                        "future:[void]", false);
  } else {
    llvmType = llvm::StructType::create(
        ctx->llctx,
        {llvm::Type::getInt64Ty(ctx->llctx), llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
         llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(), subTy->getLLVMType()->getPointerTo()},
        "future:[" + subTy->toString() + "]", false);
  }
}

FutureType* FutureType::get(QatType* subType, IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->isFuture()) {
      if (typ->asFuture()->getSubType()->isSame(subType)) {
        return typ->asFuture();
      }
    }
  }
  return new FutureType(subType, ctx);
}

QatType* FutureType::getSubType() const { return subTy; }

String FutureType::toString() const { return "future:[" + subTy->toString() + "]"; }

TypeKind FutureType::typeKind() const { return TypeKind::future; }

bool FutureType::isDestructible() const { return true; }

bool FutureType::isTypeSized() const { return true; }

void FutureType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (!vals.empty()) {
    auto* currBlock = fun->getBlock();
    auto* selfValIR = vals.front();
    if (selfValIR->isReference()) {
      selfValIR->loadImplicitPointer(ctx->builder);
    }
    auto* selfVal = selfValIR->getLLVM();
    ctx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub,
                                 ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
                                 llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u), llvm::MaybeAlign(0u),
                                 llvm::AtomicOrdering::AcquireRelease);
    SHOW("Creating zero comparison")
    auto* zeroCmp = ctx->builder.CreateICmpEQ(
        ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                ctx->builder.CreateLoad(llvm::Type::getInt64PtrTy(ctx->llctx),
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
          {new IR::Value(ctx->builder.CreateLoad(
                             llvm::PointerType::get(subTy->getLLVMType(), ctx->dataLayout->getProgramAddressSpace()),
                             ctx->builder.CreateStructGEP(llvmType, selfVal, 3u)),
                         IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary)},
          fun);
    }
    ctx->builder.CreateCall(
        freeFn->getFunctionType(), freeFn,
        {ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
                                                                ctx->builder.CreateStructGEP(llvmType, selfVal, 1u)),
                                        llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
    ctx->builder.CreateCall(
        freeFn->getFunctionType(), freeFn,
        {ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                                                ctx->builder.CreateStructGEP(llvmType, selfVal, 2u)),
                                        llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
    if (!subTy->isVoid()) {
      ctx->builder.CreateCall(
          freeFn->getFunctionType(), freeFn,
          {ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(subTy->getLLVMType()->getPointerTo(),
                                                                  ctx->builder.CreateStructGEP(llvmType, selfVal, 3u)),
                                          llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  }
}

} // namespace qat::IR