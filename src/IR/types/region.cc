#include "./region.hpp"
#include "../../show.hpp"
#include "./pointer.hpp"
#include "./struct_type.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>

#define BLOCK_HEADER_SIZE  24u
#define DATA_HEADER_SIZE   24u
#define DEFAULT_BLOCK_SIZE 4096u

namespace qat::ir {

Region* Region::get(Identifier name, Mod* parent, const VisibilityInfo& visibInfo, ir::Ctx* irCtx,
					FileRange fileRange) {
	return new Region(std::move(name), parent, visibInfo, irCtx, std::move(fileRange));
}

Region::Region(Identifier _name, Mod* _module, const VisibilityInfo& _visibInfo, ir::Ctx* irCtx, FileRange _fileRange)
	: EntityOverview("region", Json()._("moduleID", _module->get_id())._("visibility", _visibInfo), _name.range),
	  name(std::move(_name)), parent(_module), visibInfo(_visibInfo), fileRange(std::move(_fileRange)) {
	parent->regions.push_back(this);
	auto linkNames	   = parent->get_link_names().newWith(LinkNameUnit(_name.value, LinkUnitType::region), None);
	linkingName		   = linkNames.toName();
	auto& llCtx		   = irCtx->llctx;
	auto* Ty64Int	   = llvm::Type::getInt64Ty(llCtx);
	auto* zero64Bit	   = llvm::ConstantInt::get(Ty64Int, 0u);
	auto  addressSpace = irCtx->dataLayout->getProgramAddressSpace();
	blocks			   = new llvm::GlobalVariable(
		*parent->get_llvm_module(), llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), addressSpace), false,
		llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
		llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), addressSpace)),
		linkNames.newWith(LinkNameUnit("blocks", LinkUnitType::global), None).toName());
	blockCount = new llvm::GlobalVariable(
		*parent->get_llvm_module(), Ty64Int, false, llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage, zero64Bit,
		linkNames.newWith(LinkNameUnit("count", LinkUnitType::global), None).toName());
	ownFn = llvm::Function::Create(
		llvm::FunctionType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), addressSpace),
								{/* Count */ Ty64Int, /* Size of type */ Ty64Int,
								 /* Destructor Pointer */
								 llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), addressSpace)},
								false),
		llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
		linkNames.newWith(LinkNameUnit("own", LinkUnitType::function), None).toName(), parent->get_llvm_module());
	destructor = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llCtx), {}, false),
										llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
										linkNames.newWith(LinkNameUnit("end", LinkUnitType::function), None).toName(),
										parent->get_llvm_module());
	// #if NDEBUG
	// #define LogInProgram(val)
	// #else
	// #define LogInProgram(val) \
  //   SHOW(val) \
  //   ctx->builder.CreateCall(printFn->getFunctionType(), printFn, \
  //                           {ctx->builder.CreateGlobalStringPtr(String(val).append("\n"),
	//                           ctx->get_global_string_name(),
	//                           \
  //                                                               0U, ctx->getMod()->get_llvm_module())});
	//   ;
	//   (void)ir::StringSliceType::get(ctx);
	//   mod->linkNative(NativeUnit::printf);
	//   auto* printFn = mod->get_llvm_module()->getFunction("printf");
	// #endif
	{
		SHOW("Creating the own function for region: " << get_full_name())
		// FIXME - Use UIntPtr instead of u64
		auto* entry = llvm::BasicBlock::Create(llCtx, "entry", ownFn);
		irCtx->builder.SetInsertPoint(entry);
		auto* lastBlock =
			irCtx->builder.CreateAlloca(llvm::Type::getInt8Ty(llCtx)->getPointerTo(
											parent->get_llvm_module()->getDataLayout().getProgramAddressSpace()),
										nullptr, "lastBlock");
		auto* blockIndex		 = irCtx->builder.CreateAlloca(Ty64Int, nullptr, "blockIndex");
		auto* reqSize			 = irCtx->builder.CreateMul(ownFn->getArg(0u), ownFn->getArg(1u));
		auto* zeroCheckTrueBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckTrue", ownFn);
		auto* zeroCheckRestBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckRest", ownFn);
		SHOW("Creating condition to check if block count is zero")
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateICmpEQ(irCtx->builder.CreateLoad(Ty64Int, blockCount), zero64Bit), zeroCheckTrueBlock,
			zeroCheckRestBlock);
		irCtx->builder.SetInsertPoint(zeroCheckTrueBlock);
		auto* defaultBlockSize			   = llvm::ConstantInt::get(Ty64Int, DEFAULT_BLOCK_SIZE);
		auto* zeroCheckSizeCheckTrueBlock  = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckTrue", ownFn);
		auto* zeroCheckSizeCheckFalseBlock = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckFalse", ownFn);
		auto* zeroCheckSizeCheckRestBlock  = llvm::BasicBlock::Create(llCtx, "zeroCheckSizeCheckRest", ownFn);
		SHOW("Checking if requested size is greater than or equal to default block size")
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateICmpUGE(
				irCtx->builder.CreateAdd(reqSize,
										 llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE)),
				defaultBlockSize),
			zeroCheckSizeCheckTrueBlock, zeroCheckSizeCheckFalseBlock);
		irCtx->builder.SetInsertPoint(zeroCheckSizeCheckTrueBlock);
		auto* largerBlockSize =
			irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE));
		irCtx->builder.CreateBr(zeroCheckSizeCheckRestBlock);
		irCtx->builder.SetInsertPoint(zeroCheckSizeCheckFalseBlock);
		irCtx->builder.CreateBr(zeroCheckSizeCheckRestBlock);
		irCtx->builder.SetInsertPoint(zeroCheckSizeCheckRestBlock);
		auto* zeroCheckSizePhi = irCtx->builder.CreatePHI(Ty64Int, 2u);
		zeroCheckSizePhi->addIncoming(largerBlockSize, zeroCheckSizeCheckTrueBlock);
		zeroCheckSizePhi->addIncoming(defaultBlockSize, zeroCheckSizeCheckFalseBlock);
		auto  mallocName = parent->link_internal_dependency(InternalDependency::malloc, irCtx, fileRange);
		auto* mallocFn	 = parent->get_llvm_module()->getFunction(mallocName);
		// FIXME - Allow to change block size
		// FIXME - Throw/Panic if malloc fails
		auto* firstBlock = irCtx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn, {zeroCheckSizePhi});
		irCtx->builder.CreateStore(firstBlock, blocks, true);
		irCtx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 1u), blockCount);
		auto* zeroCheckBlockSizePtr =
			irCtx->builder.CreatePointerCast(firstBlock, llvm::PointerType::get(Ty64Int, addressSpace));
		irCtx->builder.CreateStore(zeroCheckSizePhi, zeroCheckBlockSizePtr);
		auto* zeroCheckBlockOccupiedPtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE)),
								   zeroCheckBlockOccupiedPtr);
		auto* ptrToVoidPtrTy = llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace);
		auto* zeroCheckBlockNextBlockPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckBlockOccupiedPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			ptrToVoidPtrTy);
		irCtx->builder.CreateStore(
			llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(llCtx), addressSpace)),
			zeroCheckBlockNextBlockPtrPtr);
		auto* zeroCheckDataCountPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 zeroCheckBlockNextBlockPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			Ty64Int->getPointerTo());
		irCtx->builder.CreateStore(ownFn->getArg(0), zeroCheckDataCountPtr);
		SHOW("Count of instances to allocate for")
		auto* zeroCheckDataTypeSizePtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckDataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		irCtx->builder.CreateStore(ownFn->getArg(1), zeroCheckDataTypeSizePtr);
		SHOW("Size of type")
		auto* zeroCheckDataDestructorPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, zeroCheckDataTypeSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		irCtx->builder.CreateStore(ownFn->getArg(2u), zeroCheckDataDestructorPtrPtr);
		auto* zeroCheckDataReturnPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 zeroCheckDataDestructorPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace));
		irCtx->builder.CreateRet(zeroCheckDataReturnPtr);
		irCtx->builder.SetInsertPoint(zeroCheckRestBlock);
		SHOW("Storing last block pointer")
		irCtx->builder.CreateStore(
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), blocks), lastBlock);
		irCtx->builder.CreateStore(zero64Bit, blockIndex);
		auto* findLastCondBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockCond", ownFn);
		auto* findLastTrueBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockTrue", ownFn);
		auto* findLastRestBlock = llvm::BasicBlock::Create(llCtx, "findLastBlockRest", ownFn);
		irCtx->builder.CreateBr(findLastCondBlock);
		irCtx->builder.SetInsertPoint(findLastCondBlock);
		SHOW("Creating condition to check block index")
		irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(Ty64Int, blockIndex),
																 irCtx->builder.CreateLoad(Ty64Int, blockCount)),
									findLastTrueBlock, findLastRestBlock);
		irCtx->builder.SetInsertPoint(findLastTrueBlock);
		SHOW("Find last true block")
		irCtx->builder.CreateStore(
			irCtx->builder.CreateLoad(
				llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
				irCtx->builder.CreatePointerCast(
					irCtx->builder.CreateInBoundsGEP(
						Ty64Int,
						irCtx->builder.CreatePointerCast(
							irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
													  lastBlock),
							Ty64Int->getPointerTo()),
						{llvm::ConstantInt::get(Ty64Int, 2u)}),
					llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace))),
			lastBlock);
		SHOW("Storing incremented block index")
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(Ty64Int, blockIndex),
															llvm::ConstantInt::get(Ty64Int, 1u)),
								   blockIndex);
		irCtx->builder.CreateBr(findLastCondBlock);
		irCtx->builder.SetInsertPoint(findLastRestBlock);
		SHOW("Setting findLastRestBlock as active")
		auto* lastBlockSizePtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), lastBlock),
			llvm::Type::getInt64Ty(llCtx)->getPointerTo(addressSpace));
		auto* lastBlockOccupiedPtr = irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt64Ty(llCtx), lastBlockSizePtr,
																	  {llvm::ConstantInt::get(Ty64Int, 1u)});
		auto* lastBlockSpaceLeftBlock = llvm::BasicBlock::Create(llCtx, "lastBlockSpaceLeftBlock", ownFn);
		auto* newBlockNeededBlock	  = llvm::BasicBlock::Create(llCtx, "newBlockNeeded", ownFn);
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateICmpUGE(
				irCtx->builder.CreateSub(irCtx->builder.CreateSub(irCtx->builder.CreateLoad(Ty64Int, lastBlockSizePtr),
																  llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE)),
										 irCtx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr)),
				irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE))),
			lastBlockSpaceLeftBlock, newBlockNeededBlock);
		irCtx->builder.SetInsertPoint(lastBlockSpaceLeftBlock);
		auto* lastBlockDataStartPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, lastBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 3u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace));
		auto* lastBlockTargetStartPtr =
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), lastBlockDataStartPtr,
											 {irCtx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr)});
		auto* lastBlockTargetCountPtr =
			irCtx->builder.CreatePointerCast(lastBlockTargetStartPtr, Ty64Int->getPointerTo());
		SHOW("Last Block: Storing count of instances")
		irCtx->builder.CreateStore(ownFn->getArg(0u), lastBlockTargetCountPtr);
		auto* lastBlockTargetSizePtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, lastBlockTargetCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		SHOW("Last Block: Storing size of one instance")
		irCtx->builder.CreateStore(ownFn->getArg(1u), lastBlockTargetSizePtr);
		auto* lastBlockTargetDestructorPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, lastBlockTargetSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		SHOW("Last Block: Storing destructor pointer")
		irCtx->builder.CreateStore(ownFn->getArg(2u), lastBlockTargetDestructorPtrPtr);
		irCtx->builder.CreateStore(
			irCtx->builder.CreateAdd(
				irCtx->builder.CreateLoad(Ty64Int, lastBlockOccupiedPtr),
				irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE))),
			lastBlockOccupiedPtr);
		SHOW("Last Block: Returning the got data pointer")
		irCtx->builder.CreateRet(irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 lastBlockTargetDestructorPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)));
		irCtx->builder.SetInsertPoint(newBlockNeededBlock);
		auto* newBlockSizeCheckTrueBlock  = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckTrue", ownFn);
		auto* newBlockSizeCheckFalseBlock = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckFalse", ownFn);
		auto* newBlockSizeCheckRestBlock  = llvm::BasicBlock::Create(llCtx, "newBlockSizeCheckRest", ownFn);
		SHOW("New Block: Size Check")
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateICmpUGE(
				irCtx->builder.CreateAdd(reqSize,
										 llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE)),
				defaultBlockSize),
			newBlockSizeCheckTrueBlock, newBlockSizeCheckFalseBlock);
		irCtx->builder.SetInsertPoint(newBlockSizeCheckTrueBlock);
		auto* newBlockLargerBlockSize =
			irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, BLOCK_HEADER_SIZE + DATA_HEADER_SIZE));
		irCtx->builder.CreateBr(newBlockSizeCheckRestBlock);
		irCtx->builder.SetInsertPoint(newBlockSizeCheckFalseBlock);
		irCtx->builder.CreateBr(newBlockSizeCheckRestBlock);
		irCtx->builder.SetInsertPoint(newBlockSizeCheckRestBlock);
		auto* newBlockSizePhi = irCtx->builder.CreatePHI(Ty64Int, 2u);
		newBlockSizePhi->addIncoming(newBlockLargerBlockSize, newBlockSizeCheckTrueBlock);
		newBlockSizePhi->addIncoming(defaultBlockSize, newBlockSizeCheckFalseBlock);
		// FIXME - Add null check
		auto* newBlockPtr = irCtx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn, {newBlockSizePhi});
		auto* lastBlockNextBlockPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt64Ty(llCtx), lastBlockOccupiedPtr,
											 {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		irCtx->builder.CreateStore(newBlockPtr, lastBlockNextBlockPtrPtr);
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(Ty64Int, blockCount),
															llvm::ConstantInt::get(Ty64Int, 1u)),
								   blockCount, true);
		auto* newBlockSizePtr = irCtx->builder.CreatePointerCast(newBlockPtr, Ty64Int->getPointerTo());
		irCtx->builder.CreateStore(newBlockSizePhi, newBlockSizePtr);
		auto* newBlockOccupiedPtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, newBlockSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(reqSize, llvm::ConstantInt::get(Ty64Int, DATA_HEADER_SIZE)),
								   newBlockOccupiedPtr);
		auto* newBlockNextBlockPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, newBlockOccupiedPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		irCtx->builder.CreateStore(
			llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)),
			newBlockNextBlockPtrPtr);
		auto* newDataCountPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 newBlockNextBlockPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			Ty64Int->getPointerTo());
		irCtx->builder.CreateStore(ownFn->getArg(0), newDataCountPtr);
		auto* newDataSizePtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, newDataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		irCtx->builder.CreateStore(ownFn->getArg(1), newDataSizePtr);
		auto* newDataDestructorPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, newDataSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		irCtx->builder.CreateStore(ownFn->getArg(2), newDataDestructorPtrPtr);
		irCtx->builder.CreateRet(irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 newDataDestructorPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)));
	}
	{
		SHOW("Creating the destructor")
		auto* entry = llvm::BasicBlock::Create(llCtx, "entry", destructor);
		irCtx->builder.SetInsertPoint(entry);
		auto* blockIndex = irCtx->builder.CreateAlloca(Ty64Int, nullptr, "blockIndex");
		auto* lastBlockPtr =
			irCtx->builder.CreateAlloca(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), nullptr, "lastBlock");
		auto* dataCursor =
			irCtx->builder.CreateAlloca(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), nullptr, "dataStart");
		auto* dataInstanceIterator = irCtx->builder.CreateAlloca(Ty64Int, nullptr, "dataInstanceIterator");
		irCtx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), blockIndex);
		SHOW("Storing the block head pointer to lastBlockPtr")
		irCtx->builder.CreateStore(
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), blocks), lastBlockPtr);
		auto* hasBlocksBlock = llvm::BasicBlock::Create(llCtx, "hasBlocks", destructor);
		auto* endBlock		 = llvm::BasicBlock::Create(llCtx, "endBlock", destructor);
		SHOW("Creating condition that block count is non zero and block head pointer is not null")
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateAnd(
				irCtx->builder.CreateICmpNE(irCtx->builder.CreateLoad(Ty64Int, blockCount),
											llvm::ConstantInt::get(Ty64Int, 0u)),
				irCtx->builder.CreateICmpNE(
					irCtx->builder.CreatePtrDiff(
						llvm::Type::getInt8Ty(llCtx),
						irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), blocks),
						llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace))),
					llvm::ConstantInt::get(Ty64Int, 0u))),
			hasBlocksBlock, endBlock);
		irCtx->builder.SetInsertPoint(hasBlocksBlock);
		auto* blockLoopMainBlock = llvm::BasicBlock::Create(llCtx, "blockLoopMain", destructor);
		irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(Ty64Int, blockIndex),
																 irCtx->builder.CreateLoad(Ty64Int, blockCount)),
									blockLoopMainBlock, endBlock);
		irCtx->builder.SetInsertPoint(blockLoopMainBlock);
		SHOW("Getting block data start pointer")
		auto* blockDataStartPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(
				Ty64Int,
				irCtx->builder.CreatePointerCast(
					irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), lastBlockPtr),
					llvm::Type::getInt64Ty(llCtx)->getPointerTo(addressSpace)),
				{llvm::ConstantInt::get(Ty64Int, 3u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace));
		SHOW("Initialising data cursor") irCtx->builder.CreateStore(blockDataStartPtr, dataCursor);
		auto* dataCursorCondBlock = llvm::BasicBlock::Create(llCtx, "dataCursorCond", destructor);
		auto* dataCursorMainBlock = llvm::BasicBlock::Create(llCtx, "dataCursorMain", destructor);
		auto* dataCursorRestBlock = llvm::BasicBlock::Create(llCtx, "dataCursorRest", destructor);
		irCtx->builder.CreateBr(dataCursorCondBlock);
		irCtx->builder.SetInsertPoint(dataCursorCondBlock);
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateICmpULT(
				irCtx->builder.CreatePtrDiff(
					llvm::Type::getInt8Ty(llCtx),
					irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), dataCursor),
					blockDataStartPtr),
				irCtx->builder.CreateLoad(
					Ty64Int, irCtx->builder.CreateInBoundsGEP(
								 Ty64Int,
								 irCtx->builder.CreatePointerCast(
									 irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
															   lastBlockPtr),
									 llvm::Type::getInt64Ty(llCtx)->getPointerTo(addressSpace)),
								 {llvm::ConstantInt::get(Ty64Int, 1u)}))),
			dataCursorMainBlock, dataCursorRestBlock);
		irCtx->builder.SetInsertPoint(dataCursorMainBlock);
		SHOW("Getting data count pointer")
		auto* dataCountPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), dataCursor),
			llvm::Type::getInt64Ty(llCtx)->getPointerTo(addressSpace));
		SHOW("Getting data count") auto* dataCount = irCtx->builder.CreateLoad(Ty64Int, dataCountPtr);
		SHOW("Getting data type size pointer")
		auto* dataTypeSizePtr =
			irCtx->builder.CreateInBoundsGEP(Ty64Int, dataCountPtr, {llvm::ConstantInt::get(Ty64Int, 1u)});
		auto* dataTypeSize = irCtx->builder.CreateLoad(Ty64Int, dataTypeSizePtr);
		SHOW("Creating destructor type")
		auto* destructorType = llvm::FunctionType::get(
			llvm::Type::getVoidTy(llCtx), {llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)}, false);
		auto* dataDestructorPtrPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(Ty64Int, dataTypeSizePtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace));
		SHOW("Got destructor pointer ref")
		auto* dataDestructorPtr = irCtx->builder.CreateBitCast(
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), dataDestructorPtrPtr),
			destructorType->getPointerTo());
		SHOW("Got destructor pointer")
		auto* blockStartDataPtr = irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
											 dataDestructorPtrPtr, {llvm::ConstantInt::get(Ty64Int, 1u)}),
			llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace));
		SHOW("Stored 0 in data instance iterator")
		irCtx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), dataInstanceIterator);
		auto* dataIndexCondBlock = llvm::BasicBlock::Create(llCtx, "dataIndexCondBlock", destructor);
		auto* dataIndexTrueBlock = llvm::BasicBlock::Create(llCtx, "dataIndexTrueBlock", destructor);
		auto* dataIndexRestBlock = llvm::BasicBlock::Create(llCtx, "dataIndexRestBlock", destructor);
		irCtx->builder.CreateBr(dataIndexCondBlock);
		irCtx->builder.SetInsertPoint(dataIndexCondBlock);
		SHOW("Creating condition to check data instance index")
		irCtx->builder.CreateCondBr(
			irCtx->builder.CreateAnd(
				irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(Ty64Int, dataInstanceIterator), dataCount),
				irCtx->builder.CreateICmpNE(
					irCtx->builder.CreatePtrDiff(
						llvm::Type::getInt8Ty(llCtx),
						irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
														 {irCtx->builder.CreateLoad(Ty64Int, dataInstanceIterator)}),
						llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace))),
					llvm::ConstantInt::get(Ty64Int, 0u))),
			dataIndexTrueBlock, dataIndexRestBlock);
		irCtx->builder.SetInsertPoint(dataIndexTrueBlock);
		SHOW("Getting current instance pointer")
		auto* currentInstancePtr =
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
											 {irCtx->builder.CreateLoad(Ty64Int, dataInstanceIterator)});
		SHOW("Calling destructor") irCtx->builder.CreateCall(destructorType, dataDestructorPtr, {currentInstancePtr});
		SHOW("Called destructor")
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(Ty64Int, dataInstanceIterator),
															llvm::ConstantInt::get(Ty64Int, 1u)),
								   dataInstanceIterator);
		SHOW("Incremented data instance iterator") irCtx->builder.CreateBr(dataIndexCondBlock);
		irCtx->builder.SetInsertPoint(dataIndexRestBlock);
		irCtx->builder.CreateStore(
			irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(llCtx), blockStartDataPtr,
											 {irCtx->builder.CreateMul(dataCount, dataTypeSize)}),
			dataCursor);
		irCtx->builder.CreateBr(dataCursorCondBlock);
		irCtx->builder.SetInsertPoint(dataCursorRestBlock);
		irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(Ty64Int, blockIndex),
															llvm::ConstantInt::get(Ty64Int, 1u)),
								   blockIndex);
		auto* doneBlockPtr =
			irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace), lastBlockPtr);
		SHOW("Updating last block pointer")
		irCtx->builder.CreateStore(
			irCtx->builder.CreateLoad(
				llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace),
				irCtx->builder.CreatePointerCast(
					irCtx->builder.CreateInBoundsGEP(
						Ty64Int,
						irCtx->builder.CreatePointerCast(lastBlockPtr,
														 llvm::Type::getInt64Ty(llCtx)->getPointerTo(addressSpace)),
						{llvm::ConstantInt::get(Ty64Int, 2u)}),
					llvm::Type::getInt8Ty(llCtx)->getPointerTo(addressSpace)->getPointerTo(addressSpace))),
			lastBlockPtr);
		auto  freeName = parent->link_internal_dependency(InternalDependency::free, irCtx, fileRange);
		auto* freeFn   = parent->get_llvm_module()->getFunction(freeName);
		irCtx->builder.CreateCall(freeFn->getFunctionType(), freeFn, {doneBlockPtr});
		SHOW("Called free function") irCtx->builder.CreateBr(hasBlocksBlock);
		irCtx->builder.SetInsertPoint(endBlock);
		irCtx->builder.CreateRetVoid();
	}
}

Identifier Region::get_name() const { return name; }

String Region::get_full_name() const { return parent->get_fullname_with_child(name.value); }

bool Region::is_accessible(const AccessInfo& reqInfo) const { return visibInfo.is_accessible(reqInfo); }

const VisibilityInfo& Region::get_visibility() const { return visibInfo; }

ir::Value* Region::ownData(ir::Type* otype, Maybe<llvm::Value*> _count, ir::Ctx* irCtx) {
	return ir::Value::get(
		irCtx->builder.CreatePointerCast(
			irCtx->builder.CreateCall(
				ownFn->getFunctionType(), ownFn,
				{(_count.has_value() ? _count.value()
									 : llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 1u)),
				 llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx),
										irCtx->dataLayout.value().getTypeAllocSize(otype->get_llvm_type())),
				 ((otype->is_struct() && otype->as_struct()->has_destructor())
					  ? irCtx->builder.CreatePointerCast(
							irCtx->builder.CreateBitCast(otype->as_struct()->get_destructor()->get_llvm_function(),
														 otype->as_struct()
															 ->get_destructor()
															 ->get_llvm_function()
															 ->getFunctionType()
															 ->getPointerTo()),
							llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo())
					  : llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo()))}),
			otype->get_llvm_type()->getPointerTo()),
		ir::MarkType::get(true, otype, false, MarkOwner::of_region(this), _count.has_value(), irCtx), false);
}

void Region::destroyObjects(ir::Ctx* irCtx) {
	irCtx->builder.CreateCall(destructor->getFunctionType(), destructor, {});
}

ir::Mod* Region::getParent() const { return parent; }

void Region::update_overview() { ovInfo._("typeID", get_id())._("fullName", get_full_name()); }

String Region::to_string() const { return parent->get_fullname_with_child(name.value); }

} // namespace qat::ir
