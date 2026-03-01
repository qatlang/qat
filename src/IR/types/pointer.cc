#include "./pointer.hpp"
#include "../control_flow.hpp"
#include "../function.hpp"
#include "./reference.hpp"
#include "./region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>

namespace qat::ir {

Vec<PtrType*> PtrType::allPtrTypes = {};

Locality Locality::in_heap() { return Locality{.origin = nullptr, .kind = LocalityKind::HEAP}; }

Locality Locality::in_static() { return Locality{.origin = nullptr, .kind = LocalityKind::STATIC}; }

Locality Locality::none() { return Locality{.origin = nullptr, .kind = LocalityKind::NONE}; }

Locality Locality::in_own() { return Locality{.origin = nullptr, .kind = LocalityKind::OWN}; }

Locality Locality::in_region_type(Region* region) {
	return Locality{.origin = region, .kind = LocalityKind::REGION_TYPE};
}

Locality Locality::in_any_region() { return Locality{.origin = nullptr, .kind = LocalityKind::ANY_REGION}; }

Locality Locality::in_prerun() { return Locality{.origin = nullptr, .kind = LocalityKind::PRERUN}; }

bool Locality::is_same(const Locality& other) const {
	if (kind == other.kind) {
		switch (kind) {
			case LocalityKind::NONE:
			case LocalityKind::STATIC:
			case LocalityKind::HEAP:
			case LocalityKind::ANY_REGION:
			case LocalityKind::PRERUN:
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC:
				return true;
			case LocalityKind::REGION_TYPE:
				return origin_as_region()->is_same(other.origin_as_region());
		}
	} else {
		return false;
	}
}

String Locality::to_string() const {
	switch (kind) {
		case LocalityKind::ANY_REGION:
			return "region";
		case LocalityKind::REGION_TYPE:
			return "region(" + origin_as_region()->to_string() + ")";
		case LocalityKind::HEAP:
			return "heap";
		case LocalityKind::NONE:
			return "";
		case LocalityKind::OWN:
			return "own";
		case LocalityKind::STATIC:
			return "static";
		case LocalityKind::PRERUN:
			return "pre";
		case LocalityKind::USE:
			return "use";
		case LocalityKind::ATOMIC:
			return "atomic";
	}
}

PtrType::PtrType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, Locality _locality, bool _hasMulti,
                 Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), locality(_locality), hasMulti(_hasMulti),
      nonNullable(_nonNullable), addressSpace(std::move(_addressSpace)) {
	if (_hasMulti) {
		linkingName = (nonNullable ? "qat'multi![" : "qat'multi:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (locality.is_none() ? "" : ",") + locality.to_string() +
		              (addressSpace.has_value() ? ("," + addressSpace.value().to_string()) : "") + "]";
		if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
			llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
		} else {
			llvmType = llvm::StructType::create(
			    {llvm::PointerType::get(irCtx->llctx, addressSpace.has_value()
			                                              ? addressSpace.value().get_number(irCtx)
			                                              : irCtx->dataLayout.getProgramAddressSpace()),
			     llvm::Type::getIntNTy(irCtx->llctx,
			                           irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()))},
			    linkingName);
		}
	} else {
		linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (locality.is_none() ? "" : ",") + locality.to_string() + "]";
		llvmType =
		    llvm::PointerType::get(irCtx->llctx, addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
		                                                                  : irCtx->dataLayout.getProgramAddressSpace());
	}
	allPtrTypes.push_back(this);
}

PtrType* PtrType::get(bool isSubtypeVariable, Type* type, bool nonNullable, Locality locality, bool hasMulti,
                      Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allPtrTypes) {
		if (typ->get_subtype()->is_same(type) && (typ->is_subtype_variable() == isSubtypeVariable) &&
		    typ->get_locality().is_same(locality) && (typ->is_multi() == hasMulti) &&
		    (typ->nonNullable == nonNullable) && ir::AddressSpace::compare(typ->get_address_space(), addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(PtrType), isSubtypeVariable, type, nonNullable, locality, hasMulti,
	                         std::move(addressSpace), irCtx);
}

PrerunValue* PtrType::get_prerun_default_value(ir::Ctx* irCtx) {
	if (has_prerun_default_value()) {
		if (is_multi()) {
			return ir::PrerunValue::get(llvm::ConstantAggregateZero::get(llvmType), this);
		} else {
			return ir::PrerunValue::get(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(get_llvm_type())),
			                            this);
		}
	} else {
		irCtx->Error("Type " + irCtx->color(to_string()) + " do not have a default value", None);
		return nullptr;
	}
}

void PtrType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	switch (locality.kind) {
		case LocalityKind::USE: {
			const auto ptrTy  = llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx));
			const auto secPtr = irCtx->builder.CreateLoad(
			    ptrTy,
			    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 0u) : second->get_llvm()));
			if (nonNullable) {
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			} else {
				const auto currBlock = fun->get_block();
				const auto trueBlock = ir::Block::create(fun, currBlock);
				const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
				restBlock->link_previous_block(currBlock);
				const auto uintPtrTy =
				    llvm::Type::getIntNTy(irCtx->llctx, irCtx->dataLayout.getPointerTypeSizeInBits(ptrTy));
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(irCtx->builder.CreatePtrToInt(secPtr, uintPtrTy),
				                                llvm::ConstantInt::get(uintPtrTy, 0u)),
				    trueBlock->get_bb(), restBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				//
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				//
				restBlock->set_active(irCtx->builder);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			}
			break;
		}
		case LocalityKind::ATOMIC: {
			const auto secPtr = irCtx->builder.CreateLoad(
			    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
			    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 0u) : second->get_llvm()));
			if (nonNullable) {
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, refCountPtr,
				                               llvm::ConstantInt::get(refCountTy, 1u), None,
				                               llvm::AtomicOrdering::Monotonic);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			} else {
				const auto currBlock = fun->get_block();
				const auto trueBlock = ir::Block::create(fun, currBlock);
				const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
				restBlock->link_previous_block(currBlock);
				const auto uintPtrTy = llvm::Type::getIntNTy(
				    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(irCtx->builder.CreatePtrToInt(secPtr, uintPtrTy),
				                                llvm::ConstantInt::get(uintPtrTy, 0u)),
				    trueBlock->get_bb(), restBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				//
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, refCountPtr,
				                               llvm::ConstantInt::get(refCountTy, 1u), None,
				                               llvm::AtomicOrdering::Monotonic);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				//
				restBlock->set_active(irCtx->builder);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			}
			break;
		}
		default:
			break;
	}
}

void PtrType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	switch (locality.kind) {
		case LocalityKind::USE: {
			const auto secPtr = irCtx->builder.CreateLoad(
			    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
			    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 0u) : second->get_llvm()));
			if (nonNullable) {
				this->destroy_value(irCtx, first, fun);
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			} else {
				const auto currBlock      = fun->get_block();
				const auto firstTrueBlock = ir::Block::create(fun, currBlock);
				const auto firstRestBlock = ir::Block::create(fun, currBlock->get_parent());
				firstRestBlock->link_previous_block(currBlock);
				const auto uintPtrTy = llvm::Type::getIntNTy(
				    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(
				        irCtx->builder.CreatePtrToInt(
				            irCtx->builder.CreateLoad(
				                llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				                (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), 0u)
				                          : first->get_llvm())),
				            uintPtrTy),
				        llvm::ConstantInt::get(uintPtrTy, 0u)),
				    firstTrueBlock->get_bb(), firstRestBlock->get_bb());
				firstTrueBlock->set_active(irCtx->builder);
				this->destroy_value(irCtx, first, fun);
				(void)ir::add_branch(irCtx->builder, firstRestBlock->get_bb());
				firstRestBlock->set_active(irCtx->builder);
				const auto secTrueBlock = ir::Block::create(fun, firstRestBlock);
				const auto restBlock    = ir::Block::create(fun, firstRestBlock->get_parent());
				restBlock->link_previous_block(firstRestBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(irCtx->builder.CreatePtrToInt(secPtr, uintPtrTy),
				                                llvm::ConstantInt::get(uintPtrTy, 0u)),
				    secTrueBlock->get_bb(), restBlock->get_bb());
				secTrueBlock->set_active(irCtx->builder);
				//
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				restBlock->set_active(irCtx->builder);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
				//
			}
			break;
		}
		case LocalityKind::ATOMIC: {
			const auto secPtr = irCtx->builder.CreateLoad(
			    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
			    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), 0u) : second->get_llvm()));
			if (nonNullable) {
				this->destroy_value(irCtx, first, fun);
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, refCountPtr,
				                               llvm::ConstantInt::get(refCountTy, 1u), None,
				                               llvm::AtomicOrdering::Monotonic);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
			} else {
				const auto currBlock = fun->get_block();
				const auto uintPtrTy = llvm::Type::getIntNTy(
				    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
				const auto firstTrueBlock = ir::Block::create(fun, currBlock);
				const auto firstRestBlock = ir::Block::create(fun, currBlock->get_parent());
				firstRestBlock->link_previous_block(currBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(
				        irCtx->builder.CreatePtrToInt(
				            irCtx->builder.CreateLoad(
				                llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				                (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), 0u)
				                          : first->get_llvm())),
				            uintPtrTy),
				        llvm::ConstantInt::get(uintPtrTy, 0u)),
				    firstTrueBlock->get_bb(), firstRestBlock->get_bb());
				firstTrueBlock->set_active(irCtx->builder);
				this->destroy_value(irCtx, first, fun);
				(void)ir::add_branch(irCtx->builder, firstRestBlock->get_bb());
				firstRestBlock->set_active(irCtx->builder);
				const auto secTrueBlock = ir::Block::create(fun, firstRestBlock);
				const auto restBlock    = ir::Block::create(fun, firstRestBlock->get_parent());
				restBlock->link_previous_block(firstRestBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(irCtx->builder.CreatePtrToInt(secPtr, uintPtrTy),
				                                llvm::ConstantInt::get(uintPtrTy, 0u)),
				    secTrueBlock->get_bb(), restBlock->get_bb());
				secTrueBlock->set_active(irCtx->builder);
				//
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, secPtr,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, refCountPtr,
				                               llvm::ConstantInt::get(refCountTy, 1u), None,
				                               llvm::AtomicOrdering::Monotonic);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				restBlock->set_active(irCtx->builder);
				irCtx->builder.CreateStore(hasMulti ? irCtx->builder.CreateLoad(llvmType, second->get_llvm()) : secPtr,
				                           first->get_llvm());
				//
			}
			break;
		}
		default:
			break;
	}
}

void PtrType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	switch (locality.kind) {
		case LocalityKind::OWN:
		case LocalityKind::USE:
		case LocalityKind::ATOMIC: {
			irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
			irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->get_llvm());
			break;
		}
		default:
			break;
	}
}

void PtrType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	switch (locality.kind) {
		case LocalityKind::OWN:
		case LocalityKind::USE:
		case LocalityKind::ATOMIC: {
			this->destroy_value(irCtx, first, fun);
			irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
			irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->get_llvm());
			break;
		}
		default:
			break;
	}
}

void PtrType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	switch (locality.kind) {
		case LocalityKind::OWN: {
			if (subType->is_destructible()) {
				if (nonNullable) {
					if (hasMulti) {
						const auto ptrVal = irCtx->builder.CreateLoad(
						    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
						    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
						              : instance->get_llvm()));
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = fun->get_block();
						const auto loopBlock = ir::Block::create(fun, currBlock);
						const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						restBlock->set_active(irCtx->builder);
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
				} else {
					const auto currBlock = fun->get_block();
					const auto uintPtrTy = llvm::Type::getIntNTy(
					    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
					const auto trueBlock = ir::Block::create(fun, currBlock);
					const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
					restBlock->link_previous_block(currBlock);
					irCtx->builder.CreateCondBr(
					    irCtx->builder.CreateICmpNE(
					        irCtx->builder.CreatePtrToInt(
					            irCtx->builder.CreateLoad(
					                llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
					                (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
					                          : instance->get_llvm())),
					            uintPtrTy),
					        llvm::ConstantInt::get(uintPtrTy, 0u)),
					    trueBlock->get_bb(), restBlock->get_bb());
					trueBlock->set_active(irCtx->builder);
					if (hasMulti) {
						const auto ptrVal = irCtx->builder.CreateLoad(
						    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
						    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
						              : instance->get_llvm()));
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = trueBlock;
						const auto loopBlock = ir::Block::create(fun, currBlock);
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
					(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
					restBlock->set_active(irCtx->builder);
				}
			}
			break;
		}
		case LocalityKind::USE: {
			if (nonNullable) {
				const auto ptrVal = irCtx->builder.CreateLoad(
				    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				              : instance->get_llvm()));
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, ptrVal,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateSub(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				if (subType->is_destructible()) {
					const auto currBlock = fun->get_block();
					const auto trueBlock = ir::Block::create(fun, currBlock);
					const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
					restBlock->link_previous_block(currBlock);
					irCtx->builder.CreateCondBr(
					    irCtx->builder.CreateICmpEQ(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
					                                llvm::ConstantInt::get(refCountTy, 0u)),
					    trueBlock->get_bb(), restBlock->get_bb());
					trueBlock->set_active(irCtx->builder);
					if (hasMulti) {
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = trueBlock;
						const auto loopBlock = ir::Block::create(fun, currBlock);
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
					(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
					restBlock->set_active(irCtx->builder);
				}
			} else {
				const auto currBlock = fun->get_block();
				const auto uintPtrTy = llvm::Type::getIntNTy(
				    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
				const auto trueBlock = ir::Block::create(fun, currBlock);
				const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
				restBlock->link_previous_block(currBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(
				        irCtx->builder.CreatePtrToInt(
				            irCtx->builder.CreateLoad(
				                llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				                (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				                          : instance->get_llvm())),
				            uintPtrTy),
				        llvm::ConstantInt::get(uintPtrTy, 0u)),
				    trueBlock->get_bb(), restBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				//
				const auto ptrVal = irCtx->builder.CreateLoad(
				    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				              : instance->get_llvm()));
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, ptrVal,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				irCtx->builder.CreateStore(irCtx->builder.CreateSub(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
				                                                    llvm::ConstantInt::get(refCountTy, 1u)),
				                           refCountPtr);
				if (subType->is_destructible()) {
					const auto oneTrueBlock = ir::Block::create(fun, trueBlock);
					irCtx->builder.CreateCondBr(
					    irCtx->builder.CreateICmpEQ(irCtx->builder.CreateLoad(refCountTy, refCountPtr),
					                                llvm::ConstantInt::get(refCountTy, 0u)),
					    oneTrueBlock->get_bb(), restBlock->get_bb());
					oneTrueBlock->set_active(irCtx->builder);
					if (hasMulti) {
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = oneTrueBlock;
						const auto loopBlock = ir::Block::create(fun, currBlock);
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
				}
				//
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				restBlock->set_active(irCtx->builder);
			}
			break;
		}
		case LocalityKind::ATOMIC: {
			if (nonNullable) {
				const auto ptrVal = irCtx->builder.CreateLoad(
				    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				              : instance->get_llvm()));
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, ptrVal,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				const auto oldValue = irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub, refCountPtr,
				                                                     llvm::ConstantInt::get(refCountTy, 1u), None,
				                                                     llvm::AtomicOrdering::AcquireRelease);
				if (subType->is_destructible()) {
					const auto currBlock = fun->get_block();
					const auto trueBlock = ir::Block::create(fun, currBlock);
					const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
					restBlock->link_previous_block(currBlock);
					irCtx->builder.CreateCondBr(
					    irCtx->builder.CreateICmpEQ(oldValue, llvm::ConstantInt::get(refCountTy, 1u)),
					    trueBlock->get_bb(), restBlock->get_bb());
					trueBlock->set_active(irCtx->builder);
					//
					if (hasMulti) {
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = trueBlock;
						const auto loopBlock = ir::Block::create(fun, currBlock);
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
					//
					(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
					restBlock->set_active(irCtx->builder);
				}
			} else {
				const auto currBlock = fun->get_block();
				const auto uintPtrTy = llvm::Type::getIntNTy(
				    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getUIntPtrType()));
				const auto trueBlock = ir::Block::create(fun, currBlock);
				const auto restBlock = ir::Block::create(fun, currBlock->get_parent());
				restBlock->link_previous_block(currBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpNE(
				        irCtx->builder.CreatePtrToInt(
				            irCtx->builder.CreateLoad(
				                llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				                (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				                          : instance->get_llvm())),
				            uintPtrTy),
				        llvm::ConstantInt::get(uintPtrTy, 0u)),
				    trueBlock->get_bb(), restBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				//
				const auto ptrVal = irCtx->builder.CreateLoad(
				    llvm::PointerType::get(irCtx->llctx, usable_address_space(irCtx)),
				    (hasMulti ? irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 0u)
				              : instance->get_llvm()));
				const auto refCountTy  = llvm::Type::getInt64Ty(irCtx->llctx);
				const auto refCountPtr = irCtx->builder.CreateInBoundsGEP(
				    refCountTy, ptrVal,
				    {llvm::ConstantInt::get(
				        llvm::Type::getIntNTy(irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(
				                                                irCtx->clangTargetInfo->getSignedSizeType())),
				        -1, true)});
				const auto oldValue = irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub, refCountPtr,
				                                                     llvm::ConstantInt::get(refCountTy, 1u), None,
				                                                     llvm::AtomicOrdering::AcquireRelease);
				if (subType->is_destructible()) {
					const auto oneTrueBlock = ir::Block::create(fun, trueBlock);
					irCtx->builder.CreateCondBr(
					    irCtx->builder.CreateICmpEQ(oldValue, llvm::ConstantInt::get(refCountTy, 1u)),
					    oneTrueBlock->get_bb(), restBlock->get_bb());
					oneTrueBlock->set_active(irCtx->builder);
					if (hasMulti) {
						const auto usizeTy = llvm::Type::getIntNTy(
						    irCtx->llctx, irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()));
						const auto ptrLen = irCtx->builder.CreateLoad(
						    usizeTy, irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), 1u));
						const auto index = fun->get_str_comparison_index(irCtx);
						irCtx->builder.CreateStore(llvm::ConstantInt::get(usizeTy, 0u), index->get_llvm());
						const auto currBlock = oneTrueBlock;
						const auto loopBlock = ir::Block::create(fun, currBlock);
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
						loopBlock->set_active(irCtx->builder);
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(irCtx->builder.CreateInBoundsGEP(
						                       subType->get_llvm_type(), ptrVal,
						                       {irCtx->builder.CreateLoad(usizeTy, index->get_llvm())}),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
						irCtx->builder.CreateStore(
						    irCtx->builder.CreateAdd(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()),
						                             llvm::ConstantInt::get(usizeTy, 1u)),
						    index->get_llvm());
						irCtx->builder.CreateCondBr(
						    irCtx->builder.CreateICmpULT(irCtx->builder.CreateLoad(usizeTy, index->get_llvm()), ptrLen),
						    loopBlock->get_bb(), restBlock->get_bb());
					} else {
						subType->destroy_value(
						    irCtx,
						    ir::Value::get(instance->get_llvm(),
						                   ir::RefType::get(true, subType, get_address_space(), irCtx), false),
						    fun);
					}
				}
				//
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				restBlock->set_active(irCtx->builder);
			}
			break;
		}
		default:
			break;
	}
}

u32 PtrType::usable_address_space(ir::Ctx* irCtx) const {
	if (addressSpace.has_value()) {
		return addressSpace->get_number(irCtx);
	} else {
		return irCtx->dataLayout.getProgramAddressSpace();
	}
}

TypeKind PtrType::type_kind() const { return TypeKind::POINTER; }

String PtrType::to_string() const {
	return String(is_multi() ? (nonNullable ? "multi![" : "multi:[") : (nonNullable ? "ptr![" : "ptr:[")) +
	       String(is_subtype_variable() ? "var " : "") + subType->to_string() + (locality.is_none() ? "" : " ") +
	       locality.to_string() + "]";
}

} // namespace qat::ir
