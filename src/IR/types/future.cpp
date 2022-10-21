#include "./future.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../logic.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

FutureType::FutureType(QatType* _subType, IR::Context* ctx) : subTy(_subType) {
  if (subTy->isVoid()) {
    llvmType = llvm::StructType::create(ctx->llctx,
                                        {llvm::Type::getInt64Ty(ctx->llctx),
                                         llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
                                         llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo()},
                                        "future void", false);
  } else {
    llvmType = llvm::StructType::create(
        ctx->llctx,
        {llvm::Type::getInt64Ty(ctx->llctx), llvm::Type::getInt64Ty(ctx->llctx)->getPointerTo(),
         llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(), subTy->getLLVMType()->getPointerTo()},
        "future " + subTy->toString(), false);
  }
  // SHOW("Creating destructor for " + toString());
  // auto* futureRefTy = llvmType->getPointerTo();
  // destructor = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx->llctx), {futureRefTy},
  // false),
  //                                     llvm::GlobalValue::LinkageTypes::WeakAnyLinkage, toString() + "'" + "end",
  //                                     ctx->getMod()->getLLVMModule());
  // SHOW("Created entry block")
  // auto* destrEntry = llvm::BasicBlock::Create(ctx->llctx, "entry", destructor);
  // SHOW("Is entry block" << destrEntry->isEntryBlock());
  // SHOW("Future Destructor entry block address is: " << destrEntry)
  // ctx->builder.SetInsertPoint(destrEntry);
  // SHOW("Creating self pointer allocation")
  // auto* selfAlloc = ctx->builder.CreateAlloca(futureRefTy, nullptr, "futureValue");
  // SHOW("Self alloca address: " << selfAlloc)
  // SHOW("Storing self pointer value from arg")
  // ctx->builder.CreateStore(destructor->getArg(0), selfAlloc);
  // SHOW("Getting self pointer")
  // auto* selfVal = ctx->builder.CreateLoad(futureRefTy, selfAlloc);
  // ctx->builder.CreateStore(
  //     ctx->builder.CreateSub(ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
  //                                                    ctx->builder.CreateStructGEP(llvmType, selfVal, 1u), true),
  //                            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)),
  //     ctx->builder.CreateStructGEP(llvmType, selfVal, 1u), true);
  // SHOW("Creating zero comparison")
  // auto* zeroCmp = ctx->builder.CreateICmpEQ(
  //     ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx), ctx->builder.CreateStructGEP(llvmType, selfVal,
  //     1u)), llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u));
  // auto* trueBlock = llvm::BasicBlock::Create(ctx->llctx, "isCounterZero", destructor);
  // auto* restBlock = llvm::BasicBlock::Create(ctx->llctx, "rest", destructor);
  // ctx->getMod()->linkNative(NativeUnit::free);
  // ctx->builder.CreateCondBr(zeroCmp, trueBlock, restBlock);
  // ctx->builder.SetInsertPoint(trueBlock);
  // auto* free = ctx->getMod()->getLLVMModule()->getFunction("free");
  // ctx->builder.CreateCall(
  //     free->getFunctionType(), free,
  //     {ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
  //                                                             ctx->builder.CreateStructGEP(llvmType, selfVal, 2u)),
  //                                     llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
  // if (!subTy->isVoid()) {
  //   ctx->builder.CreateCall(
  //       free->getFunctionType(), free,
  //       {ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(subTy->getLLVMType()->getPointerTo(),
  //                                                               ctx->builder.CreateStructGEP(llvmType, selfVal, 3u)),
  //                                       llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
  // }
  // ctx->builder.CreateBr(restBlock);
  // ctx->builder.SetInsertPoint(restBlock);
  // ctx->builder.CreateRetVoid();
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

String FutureType::toString() const { return "future " + subTy->toString(); }

TypeKind FutureType::typeKind() const { return TypeKind::future; }

} // namespace qat::IR