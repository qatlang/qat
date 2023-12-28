#include "./region.hpp"
#include "../../cli/version.hpp"
#include "../../show.hpp"
#include "pointer.hpp"
#include "string_slice.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"

#define BLOCK_HEADER_SIZE  24u
#define DATA_HEADER_SIZE   24u
#define DEFAULT_BLOCK_SIZE 4096u

namespace qat::IR {

Region* Region::get(Identifier name, QatModule* parent, const VisibilityInfo& visibInfo, IR::Context* ctx,
                    FileRange fileRange) {
  return new Region(std::move(name), parent, visibInfo, ctx, std::move(fileRange));
}

Region::Region(Identifier _name, QatModule* _module, const VisibilityInfo& _visibInfo, IR::Context* ctx,
               FileRange _fileRange)
    : EntityOverview("region", Json()._("moduleID", _module->getID())._("visibility", _visibInfo), _name.range),
      name(std::move(_name)), parent(_module), visibInfo(_visibInfo), fileRange(std::move(_fileRange)) {
  parent->regions.push_back(this);
  auto linkNames  = parent->getLinkNames().newWith(LinkNameUnit(_name.value, LinkUnitType::region), None);
  linkingName     = linkNames.toName();
  auto& llCtx     = ctx->llctx;
  auto* Ty64Int   = llvm::Type::getInt64Ty(llCtx);
  auto* zero64Bit = llvm::ConstantInt::get(Ty64Int, 0u);
  auto* mod       = ctx->getMod();
  blocks          = new llvm::GlobalVariable(
      *mod->getLLVMModule(),
      llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), ctx->dataLayout->getProgramAddressSpace()), false,
      llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
      llvm::ConstantPointerNull::get(
          llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), ctx->dataLayout->getProgramAddressSpace())),
      linkNames.newWith(LinkNameUnit("blocks", LinkUnitType::global), None).toName());
  blockCount = new llvm::GlobalVariable(*mod->getLLVMModule(), Ty64Int, false,
                                        llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage, zero64Bit,
                                        linkNames.newWith(LinkNameUnit("count", LinkUnitType::global), None).toName());
  ownFn      = llvm::Function::Create(
      llvm::FunctionType::get(
          llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), ctx->dataLayout->getProgramAddressSpace()),
          {/* Count */ Ty64Int, /* Size of type */ Ty64Int,
           /* Destructor Pointer */
           llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), ctx->dataLayout->getProgramAddressSpace())},
          false),
      llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
      linkNames.newWith(LinkNameUnit("own", LinkUnitType::function), None).toName(), mod->getLLVMModule());
  destructor = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {}, false),
                                      llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
                                      linkNames.newWith(LinkNameUnit("end", LinkUnitType::function), None).toName(),
                                      mod->getLLVMModule());
  // #if NDEBUG
  // #define LogInProgram(val)
  // #else
  // #define LogInProgram(val) \
  //   SHOW(val) \
  //   ctx->builder.CreateCall(printFn->getFunctionType(), printFn, \
  //                           {ctx->builder.CreateGlobalStringPtr(String(val).append("\n"), ctx->getGlobalStringName(),
  //                           \
  //                                                               0U, ctx->getMod()->getLLVMModule())});
  //   ;
  //   (void)IR::StringSliceType::get(ctx);
  //   mod->linkNative(NativeUnit::printf);
  //   auto* printFn = mod->getLLVMModule()->getFunction("printf");
  // #endif
  {
    SHOW("Creating the own function for region: " << getFullName())
    // FIXME - Use UIntPtr instead of u64
    auto* entry = llvm::BasicBlock::Create(llCtx, "entry", ownFn);
    ctx->builder.SetInsertPoint(entry);
    auto* lastBlock          = ctx->builder.CreateAlloca(llvm::Type::getInt8PtrTy(llCtx), nullptr, "lastBlock");
    auto* blockIndex         = ctx->builder.CreateAlloca(Ty64Int, nullptr, "blockIndex");
    auto* reqSize            = ctx->builder.CreateMul(ownFn->getArg(0u), ownFn->getArg(1u));
    auto* zeroCheckTrueBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckTrue", ownFn);
    auto* zeroCheckRestBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckRest", ownFn);
    SHOW("Creating condition to check if block count is zero")
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(Ty64Int, blockCount), zero64Bit),
                              zeroCheckTrueBlock, zeroCheckRestBlock);
    ctx->builder.SetInsertPoint(zeroCheckTrueBlock);
    auto* defaultBlockSize             = llvm::ConstantInt::get(Ty64Int, DEFAULT_BLOCK_SIZE);
    auto* zeroCheckSizeCheckTrueBlock  = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckTrue", ownFn);
    auto* zeroCheckSizeCheckFalseBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckFalse", ownFn);
    auto* zeroCheckSizeCheckRestBlock  = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckRest", ownFn);
    SHOW("Checking if requested size is greater than or equal to default block size")
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpUGE(
            ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE)),
            defaultBlockSize),
        zeroCheckSizeCheckTrueBlock, zeroCheckSizeCheckFalseBlock);
    ctx->builder.SetInsertPoint(zeroCheckSizeCheckTrueBlock);
    auto* largerBlockSize =
        ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE));
    ctx->builder.CreateBr(zeroCheckSizeCheckRestBlock);
    ctx->builder.SetInsertPoint(zeroCheckSizeCheckFalseBlock);
    ctx->builder.CreateBr(zeroCheckSizeCheckRestBlock);
    ctx->builder.SetInsertPoint(zeroCheckSizeCheckRestBlock);
    auto* zeroCheckSizePhi = ctx->builder.CreatePHI(Ty64Int, 2u);
    zeroCheckSizePhi->addIncoming(largerBlockSize, zeroCheckSizeCheckTrueBlock);
    zeroCheckSizePhi->addIncoming(defaultBlockSize, zeroCheckSizeCheckFalseBlock);
    mod->linkNative(NativeUnit::malloc);
    auto* mallocFn = mod->getLLVMModule()->getFunction("malloc");
    // FIXME - Allow to change block size
    // FIXME - Throw/Panic if malloc fails
    auto* firstBlock = ctx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn, {zeroCheckSizePhi});
    ctx->builder.CreateStore(firstBlock, blocks, true);
    ctx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 1u), blockCount);
    auto* zeroCheckBlockSizePtr = ctx->builder.CreatePointerCast(
        firstBlock, llvm::PointerType::get(Ty64Int, ctx->dataLayout->getProgramAddressSpace()));
    ctx->builder.CreateStore(zeroCheckSizePhi, zeroCheckBlockSizePtr);
    auto* zeroCheckBlockOccupiedPtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    ctx->builder.CreateStore(ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE)),
                             zeroCheckBlockOccupiedPtr);
    auto* ptrToVoidPtrTy                = llvm::Type::getInt8PtrTy(llCtx)->getPointerTo();
    auto* zeroCheckBlockNextBlockPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckBlockOccupiedPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        ptrToVoidPtrTy);
    ctx->builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::get(
                                 llvm::Type::getInt8Ty(llCtx), ctx->dataLayout->getProgramAddressSpace())),
                             zeroCheckBlockNextBlockPtrPtr);
    auto* zeroCheckDataCountPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), zeroCheckBlockNextBlockPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        Ty64Int->getPointerTo());
    ctx->builder.CreateStore(ownFn->getArg(0), zeroCheckDataCountPtr);
    SHOW("Count of instances to allocate for")
    auto* zeroCheckDataTypeSizePtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckDataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    ctx->builder.CreateStore(ownFn->getArg(1), zeroCheckDataTypeSizePtr);
    SHOW("Size of type")
    auto* zeroCheckDataDestructorPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckDataTypeSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    ctx->builder.CreateStore(ownFn->getArg(2u), zeroCheckDataDestructorPtrPtr);
    auto* zeroCheckDataReturnPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), zeroCheckDataDestructorPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx));
    ctx->builder.CreateRet(zeroCheckDataReturnPtr);
    ctx->builder.SetInsertPoint(zeroCheckRestBlock);
    SHOW("Storing last block pointer")
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), blocks), lastBlock);
    ctx->builder.CreateStore(zero64Bit, blockIndex);
    auto* findLastCondBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockCond", ownFn);
    auto* findLastTrueBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockTrue", ownFn);
    auto* findLastRestBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockRest", ownFn);
    ctx->builder.CreateBr(findLastCondBlock);
    ctx->builder.SetInsertPoint(findLastCondBlock);
    SHOW("Creating condition to check block index")
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, blockIndex),
                                                         ctx->builder.CreateLoad(Ty64Int, blockCount)),
                              findLastTrueBlock, findLastRestBlock);
    ctx->builder.SetInsertPoint(findLastTrueBlock);
    SHOW("Find last true block")
    ctx->builder.CreateStore(
        ctx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(llCtx),
            ctx->builder.CreatePointerCast(
                ctx->builder.CreateInBoundsGEP(
                    Ty64Int,
                    ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), lastBlock),
                                                   Ty64Int->getPointerTo()),
                    {llvm::ConstantInt::get(Ty64Int, 2u)}),
                llvm::Type::getInt8PtrTy(llCtx)->getPointerTo())),
        lastBlock);
    SHOW("Storing incremented block index")
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, blockIndex), llvm::ConstantInt::get(Ty64Int, 1u)),
        blockIndex);
    ctx->builder.CreateBr(findLastCondBlock);
    ctx->builder.SetInsertPoint(findLastRestBlock);
    SHOW("Setting findLastRestBlock as active")
    auto* lastBlockSizePtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), lastBlock), llvm::Type::getInt64PtrTy(llCtx));
    auto* lastBlockOccupiedPtr    = ctx->builder.CreateInBoundsGEP(llvm::Type::getInt64Ty(llCtx), lastBlockSizePtr,
                                                                   {llvm::ConstantInt::get(Ty64Int, 1u)});
    auto* lastBlockSpaceLeftBlock = llvm::BasicBlock::Create(llCtx, "lastBlockSpaceLeftBlock", ownFn);
    auto* newBlockNeededBlock     = llvm::BasicBlock::Create(llCtx, "newBlockNeeded", ownFn);
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpUGE(
            ctx->builder.CreateSub(ctx->builder.CreateSub(ctx->builder.CreateLoad(Ty64Int, lastBlockSizePtr),
                                                          llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE)),
                                   ctx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr)),
            ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE))),
        lastBlockSpaceLeftBlock, newBlockNeededBlock);
    ctx->builder.SetInsertPoint(lastBlockSpaceLeftBlock);
    auto* lastBlockDataStartPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, lastBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 3u)}),
        llvm::Type::getInt8PtrTy(llCtx));
    auto* lastBlockTargetStartPtr = ctx->builder.CreateInBoundsGEP(
        llvm::Type::getInt8Ty(llCtx), lastBlockDataStartPtr, {ctx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr)});
    auto* lastBlockTargetCountPtr = ctx->builder.CreatePointerCast(lastBlockTargetStartPtr, Ty64Int->getPointerTo());
    SHOW("Last Block: Storing count of instances")
    ctx->builder.CreateStore(ownFn->getArg(0u), lastBlockTargetCountPtr);
    auto* lastBlockTargetSizePtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, lastBlockTargetCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    SHOW("Last Block: Storing size of one instance")
    ctx->builder.CreateStore(ownFn->getArg(1u), lastBlockTargetSizePtr);
    auto* lastBlockTargetDestructorPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, lastBlockTargetSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    SHOW("Last Block: Storing destructor pointer")
    ctx->builder.CreateStore(ownFn->getArg(2u), lastBlockTargetDestructorPtrPtr);
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr),
                               ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE))),
        lastBlockOccupiedPtr);
    SHOW("Last Block: Returning the got data pointer")
    ctx->builder.CreateRet(ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), lastBlockTargetDestructorPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)));
    ctx->builder.SetInsertPoint(newBlockNeededBlock);
    auto* newBlockSizeCheckTrueBlock  = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckTrue", ownFn);
    auto* newBlockSizeCheckFalseBlock = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckFalse", ownFn);
    auto* newBlockSizeCheckRestBlock  = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckRest", ownFn);
    SHOW("New Block: Size Check")
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpUGE(
            ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE)),
            defaultBlockSize),
        newBlockSizeCheckTrueBlock, newBlockSizeCheckFalseBlock);
    ctx->builder.SetInsertPoint(newBlockSizeCheckTrueBlock);
    auto* newBlockLargerBlockSize =
        ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE));
    ctx->builder.CreateBr(newBlockSizeCheckRestBlock);
    ctx->builder.SetInsertPoint(newBlockSizeCheckFalseBlock);
    ctx->builder.CreateBr(newBlockSizeCheckRestBlock);
    ctx->builder.SetInsertPoint(newBlockSizeCheckRestBlock);
    auto* newBlockSizePhi = ctx->builder.CreatePHI(Ty64Int, 2u);
    newBlockSizePhi->addIncoming(newBlockLargerBlockSize, newBlockSizeCheckTrueBlock);
    newBlockSizePhi->addIncoming(defaultBlockSize, newBlockSizeCheckFalseBlock);
    // FIXME - Add null check
    auto* newBlockPtr              = ctx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn, {newBlockSizePhi});
    auto* lastBlockNextBlockPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt64Ty(llCtx), lastBlockOccupiedPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    ctx->builder.CreateStore(newBlockPtr, lastBlockNextBlockPtrPtr);
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, blockCount), llvm::ConstantInt::get(Ty64Int, 1u)),
        blockCount, true);
    auto* newBlockSizePtr = ctx->builder.CreatePointerCast(newBlockPtr, Ty64Int->getPointerTo());
    ctx->builder.CreateStore(newBlockSizePhi, newBlockSizePtr);
    auto* newBlockOccupiedPtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, newBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    ctx->builder.CreateStore(ctx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE)),
                             newBlockOccupiedPtr);
    auto* newBlockNextBlockPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, newBlockOccupiedPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    ctx->builder.CreateStore(llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(llCtx)), newBlockNextBlockPtrPtr);
    auto* newDataCountPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), newBlockNextBlockPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        Ty64Int->getPointerTo());
    ctx->builder.CreateStore(ownFn->getArg(0), newDataCountPtr);
    auto* newDataSizePtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, newDataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    ctx->builder.CreateStore(ownFn->getArg(1), newDataSizePtr);
    auto* newDataDestructorPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, newDataSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    ctx->builder.CreateStore(ownFn->getArg(2), newDataDestructorPtrPtr);
    ctx->builder.CreateRet(ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), newDataDestructorPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)));
  }
  {
    SHOW("Creating the destructor")
    auto* entry = llvm::BasicBlock::Create(llCtx, "entry", destructor);
    ctx->builder.SetInsertPoint(entry);
    auto* blockIndex           = ctx->builder.CreateAlloca(Ty64Int, nullptr, "blockIndex");
    auto* lastBlockPtr         = ctx->builder.CreateAlloca(llvm::Type::getInt8PtrTy(llCtx), nullptr, "lastBlock");
    auto* dataCursor           = ctx->builder.CreateAlloca(llvm::Type::getInt8PtrTy(llCtx), nullptr, "dataStart");
    auto* dataInstanceIterator = ctx->builder.CreateAlloca(Ty64Int, nullptr, "dataInstanceIterator");
    ctx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), blockIndex);
    SHOW("Storing the block head pointer to lastBlockPtr")
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), blocks), lastBlockPtr);
    auto* hasBlocksBlock = llvm::BasicBlock::Create(llCtx, "hasBlocks", destructor);
    auto* endBlock       = llvm::BasicBlock::Create(llCtx, "endBlock", destructor);
    SHOW("Creating condition that block count is non zero and block head pointer is not null")
    ctx->builder.CreateCondBr(
        ctx->builder.CreateAnd(
            ctx->builder.CreateICmpNE(ctx->builder.CreateLoad(Ty64Int, blockCount),
                                      llvm::ConstantInt::get(Ty64Int, 0u)),
            ctx->builder.CreateICmpNE(
                ctx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(llCtx),
                                           ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), blocks),
                                           llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(llCtx))),
                llvm::ConstantInt::get(Ty64Int, 0u))),
        hasBlocksBlock, endBlock);
    ctx->builder.SetInsertPoint(hasBlocksBlock);
    auto* blockLoopMainBlock = llvm::BasicBlock::Create(llCtx, "blockLoopMain", destructor);
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, blockIndex),
                                                         ctx->builder.CreateLoad(Ty64Int, blockCount)),
                              blockLoopMainBlock, endBlock);
    ctx->builder.SetInsertPoint(blockLoopMainBlock);
    SHOW("Getting block data start pointer")
    auto* blockDataStartPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(
            Ty64Int,
            ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), lastBlockPtr),
                                           llvm::Type::getInt64PtrTy(llCtx)),
            {llvm::ConstantInt::get(Ty64Int, 3u)}),
        llvm::Type::getInt8PtrTy(llCtx));
    SHOW("Initialising data cursor") ctx->builder.CreateStore(blockDataStartPtr, dataCursor);
    auto* dataCursorCondBlock = llvm::BasicBlock::Create(llCtx, "dataCursorCond", destructor);
    auto* dataCursorMainBlock = llvm::BasicBlock::Create(llCtx, "dataCursorMain", destructor);
    auto* dataCursorRestBlock = llvm::BasicBlock::Create(llCtx, "dataCursorRest", destructor);
    ctx->builder.CreateBr(dataCursorCondBlock);
    ctx->builder.SetInsertPoint(dataCursorCondBlock);
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpULT(
            ctx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(llCtx),
                                       ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), dataCursor),
                                       blockDataStartPtr),
            ctx->builder.CreateLoad(Ty64Int,
                                    ctx->builder.CreateInBoundsGEP(
                                        Ty64Int,
                                        ctx->builder.CreatePointerCast(
                                            ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), lastBlockPtr),
                                            llvm::Type::getInt64PtrTy(llCtx)),
                                        {llvm::ConstantInt::get(Ty64Int, 1u)}))),
        dataCursorMainBlock, dataCursorRestBlock);
    ctx->builder.SetInsertPoint(dataCursorMainBlock);
    SHOW("Getting data count pointer")
    auto* dataCountPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), dataCursor), llvm::Type::getInt64PtrTy(llCtx));
    SHOW("Getting data count") auto* dataCount = ctx->builder.CreateLoad(Ty64Int, dataCountPtr);
    SHOW("Getting data type size pointer")
    auto* dataTypeSizePtr =
        ctx->builder.CreateInBoundsGEP(Ty64Int, dataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
    auto* dataTypeSize = ctx->builder.CreateLoad(Ty64Int, dataTypeSizePtr);
    SHOW("Creating destructor type")
    auto* destructorType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {llvm::Type::getInt8PtrTy(llCtx)}, false);
    auto* dataDestructorPtrPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(Ty64Int, dataTypeSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx)->getPointerTo());
    SHOW("Got destructor pointer ref")
    auto* dataDestructorPtr = ctx->builder.CreateBitCast(
        ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), dataDestructorPtrPtr), destructorType->getPointerTo());
    SHOW("Got destructor pointer")
    auto* blockStartDataPtr = ctx->builder.CreatePointerCast(
        ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8PtrTy(llCtx), dataDestructorPtrPtr,
                                       {llvm::ConstantInt::get(Ty64Int, 1u)}),
        llvm::Type::getInt8PtrTy(llCtx));
    SHOW("Stored 0 in data instance iterator")
    ctx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), dataInstanceIterator);
    auto* dataIndexCondBlock = llvm::BasicBlock::Create(llCtx, "dataIndexCondBlock", destructor);
    auto* dataIndexTrueBlock = llvm::BasicBlock::Create(llCtx, "dataIndexTrueBlock", destructor);
    auto* dataIndexRestBlock = llvm::BasicBlock::Create(llCtx, "dataIndexRestBlock", destructor);
    ctx->builder.CreateBr(dataIndexCondBlock);
    ctx->builder.SetInsertPoint(dataIndexCondBlock);
    SHOW("Creating condition to check data instance index")
    ctx->builder.CreateCondBr(
        ctx->builder.CreateAnd(
            ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, dataInstanceIterator), dataCount),
            ctx->builder.CreateICmpNE(
                ctx->builder.CreatePtrDiff(
                    llvm::Type::getInt8Ty(llCtx),
                    ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
                                                   {ctx->builder.CreateLoad(Ty64Int, dataInstanceIterator)}),
                    llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(llCtx))),
                llvm::ConstantInt::get(Ty64Int, 0u))),
        dataIndexTrueBlock, dataIndexRestBlock);
    ctx->builder.SetInsertPoint(dataIndexTrueBlock);
    SHOW("Getting current instance pointer")
    auto* currentInstancePtr = ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
                                                              {ctx->builder.CreateLoad(Ty64Int, dataInstanceIterator)});
    SHOW("Calling destructor") ctx->builder.CreateCall(destructorType, dataDestructorPtr, {currentInstancePtr});
    SHOW("Called destructor")
    ctx->builder.CreateStore(ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, dataInstanceIterator),
                                                    llvm::ConstantInt::get(Ty64Int, 1u)),
                             dataInstanceIterator);
    SHOW("Incremented data instance iterator") ctx->builder.CreateBr(dataIndexCondBlock);
    ctx->builder.SetInsertPoint(dataIndexRestBlock);
    ctx->builder.CreateStore(ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
                                                            {ctx->builder.CreateMul(dataCount, dataTypeSize)}),
                             dataCursor);
    ctx->builder.CreateBr(dataCursorCondBlock);
    ctx->builder.SetInsertPoint(dataCursorRestBlock);
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, blockIndex), llvm::ConstantInt::get(Ty64Int, 1u)),
        blockIndex);
    auto* doneBlockPtr = ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(llCtx), lastBlockPtr);
    SHOW("Updating last block pointer")
    ctx->builder.CreateStore(
        ctx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(llCtx),
            ctx->builder.CreatePointerCast(
                ctx->builder.CreateInBoundsGEP(
                    Ty64Int, ctx->builder.CreatePointerCast(lastBlockPtr, llvm::Type::getInt64PtrTy(llCtx)),
                    {llvm::ConstantInt::get(Ty64Int, 2u)}),
                llvm::Type::getInt8PtrTy(llCtx)->getPointerTo())),
        lastBlockPtr);
    mod->linkNative(NativeUnit::free);
    auto* freeFn = mod->getLLVMModule()->getFunction("free");
    ctx->builder.CreateCall(freeFn->getFunctionType(), freeFn, {doneBlockPtr});
    SHOW("Called free function") ctx->builder.CreateBr(hasBlocksBlock);
    ctx->builder.SetInsertPoint(endBlock);
    ctx->builder.CreateRetVoid();
  }
}

Identifier Region::getName() const { return name; }

String Region::getFullName() const { return parent->getFullNameWithChild(name.value); }

bool Region::isAccessible(const AccessInfo& reqInfo) const { return visibInfo.isAccessible(reqInfo); }

const VisibilityInfo& Region::getVisibility() const { return visibInfo; }

IR::Value* Region::ownData(IR::QatType* otype, Maybe<llvm::Value*> _count, IR::Context* ctx) {
  return new IR::Value(
      ctx->builder.CreatePointerCast(
          ctx->builder.CreateCall(
              ownFn->getFunctionType(), ownFn,
              {(_count.has_value() ? _count.value() : llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)),
               llvm::ConstantInt::get(
                   llvm::Type::getInt64Ty(ctx->llctx),
                   ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSize(otype->getLLVMType())),
               ((otype->isCoreType() && otype->asCore()->hasDestructor())
                    ? ctx->builder.CreatePointerCast(
                          ctx->builder.CreateBitCast(
                              otype->asCore()->getDestructor()->getLLVMFunction(),
                              otype->asCore()->getDestructor()->getLLVMFunction()->getFunctionType()->getPointerTo()),
                          llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())
                    : llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()))}),
          otype->getLLVMType()->getPointerTo()),
      IR::PointerType::get(true, otype, false, PointerOwner::OfRegion(this), _count.has_value(), ctx), false,
      IR::Nature::temporary);
}

void Region::destroyObjects(IR::Context* ctx) {
  ctx->builder.CreateCall(destructor->getFunctionType(), destructor, {});
}

void Region::updateOverview() { ovInfo._("typeID", getID())._("fullName", getFullName()); }

String Region::toString() const { return parent->getFullNameWithChild(name.value); }

} // namespace qat::IR