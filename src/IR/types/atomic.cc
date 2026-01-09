#include "./atomic.hpp"
#include "../context.hpp"

namespace qat::ir {

llvm::AtomicOrdering AtomicType::get_llvm_ordering(ir::Ctx* irCtx) const {
	if (ordering.has_value() || irCtx->atomicScopeOrdering.has_value()) {
		switch (ordering.value_or(irCtx->atomicScopeOrdering.value().first)) {
			case AtomicOrdering::ACQUIRE:
				return llvm::AtomicOrdering::Acquire;
			case AtomicOrdering::RELEASE:
				return llvm::AtomicOrdering::Release;
			case AtomicOrdering::ACQUIRE_AND_RELEASE:
				return llvm::AtomicOrdering::AcquireRelease;
			case AtomicOrdering::RELAXED:
				return llvm::AtomicOrdering::Monotonic;
			case AtomicOrdering::SEQUENTIALLY_CONSISTENT:
				return llvm::AtomicOrdering::SequentiallyConsistent;
			case AtomicOrdering::UNORDERED:
				return llvm::AtomicOrdering::Unordered;
		}
	} else {
		return llvm::AtomicOrdering::SequentiallyConsistent;
	}
}

void AtomicType::default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function*) {
	auto store = irCtx->builder.CreateStore(subType->get_prerun_default_value(irCtx)->get_llvm(), instance->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

void AtomicType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function*) {
	auto load = irCtx->builder.CreateLoad(llvmType, second->get_llvm());
	load->setAtomic(get_llvm_ordering(irCtx));
	auto store = irCtx->builder.CreateStore(load, first->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

void AtomicType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function*) {
	auto load = irCtx->builder.CreateLoad(llvmType, second->get_llvm());
	load->setAtomic(get_llvm_ordering(irCtx));
	auto store = irCtx->builder.CreateStore(load, first->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

void AtomicType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function*) {
	auto store = irCtx->builder.CreateStore(
	    irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::BinOp::Xchg, second->get_llvm(),
	                                   llvm::Constant::getNullValue(llvmType), None, get_llvm_ordering(irCtx)),
	    first->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

void AtomicType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function*) {
	auto store = irCtx->builder.CreateStore(
	    irCtx->builder.CreateAtomicRMW(llvm::AtomicRMWInst::BinOp::Xchg, second->get_llvm(),
	                                   llvm::Constant::getNullValue(llvmType), None, get_llvm_ordering(irCtx)),
	    first->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

void AtomicType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function*) {
	auto store = irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), instance->get_llvm());
	store->setAtomic(get_llvm_ordering(irCtx));
}

} // namespace qat::ir
