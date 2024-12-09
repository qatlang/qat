#include "./unsigned.hpp"
#include "../context.hpp"
#include "../value.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

UnsignedType::UnsignedType(u64 _bitWidth, ir::Ctx* _ctx, bool _isBool)
	: bitWidth(_bitWidth), isTypeBool(_isBool), irCtx(_ctx) {
	llvmType	= llvm::IntegerType::get(irCtx->llctx, bitWidth);
	linkingName = "qat'" + to_string();
}

UnsignedType* UnsignedType::create(u64 bits, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_unsigned_integer()) {
			auto candidate = typ->as_unsigned_integer();
			if (candidate->is_bitWidth(bits) && !candidate->isTypeBool) {
				return typ->as_unsigned_integer();
			}
		}
	}
	return std::construct_at(OwnNormal(UnsignedType), bits, irCtx, false);
}

UnsignedType* UnsignedType::create_bool(ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_unsigned_integer()) {
			if (typ->as_unsigned_integer()->isTypeBool) {
				return typ->as_unsigned_integer();
			}
		}
	}
	return std::construct_at(OwnNormal(UnsignedType), 1u, irCtx, true);
}

ir::PrerunValue* UnsignedType::get_prerun_default_value(ir::Ctx* irCtx) {
	return ir::PrerunValue::get(llvm::ConstantInt::get(get_llvm_type(), 0u, false), this);
}

Maybe<String> UnsignedType::to_prerun_generic_string(ir::PrerunValue* val) const {
	llvm::ConstantInt* digit = nullptr;
	auto			   len	 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(get_llvm_type()->getContext()), 1u, false);
	auto			   value = val->get_llvm_constant();
	auto			   temp	 = value;
	while (llvm::cast<llvm::ConstantInt>(
			   llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_UGE, temp,
													llvm::ConstantInt::get(value->getType(), 10u, false)))
			   ->getValue()
			   .getBoolValue()) {
		len			= llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
			llvm::ConstantExpr::getAdd(len, llvm::ConstantInt::get(len->getType(), 1u, false)),
			irCtx->dataLayout.value()));
		auto* radix = llvm::ConstantInt::get(temp->getType(), 10u, false);
		temp		= llvm::ConstantFoldBinaryOpOperands(
			   llvm::Instruction::BinaryOps::UDiv,
			   llvm::ConstantExpr::getSub(temp,
											  llvm::ConstantFoldBinaryOpOperands(llvm::Instruction::BinaryOps::URem, temp,
																				 radix, irCtx->dataLayout.value())),
			   radix, irCtx->dataLayout.value());
	}
	Vec<llvm::ConstantInt*> resultDigits(*len->getValue().getRawData(), nullptr);
	do {
		digit										= llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
			  llvm::Instruction::BinaryOps::URem, value, llvm::ConstantInt::get(value->getType(), 10u, false),
			  irCtx->dataLayout.value()));
		len											= llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
			llvm::ConstantExpr::getSub(len, llvm::ConstantInt::get(len->getType(), 1u, false)),
			irCtx->dataLayout.value()));
		resultDigits[*len->getValue().getRawData()] = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldIntegerCast(
			digit, llvm::Type::getInt8Ty(irCtx->llctx), false, irCtx->dataLayout.value()));
		value										= llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
			  llvm::Instruction::BinaryOps::UDiv, llvm::ConstantExpr::getSub(value, digit),
			  llvm::ConstantInt::get(value->getType(), 10u, false), irCtx->dataLayout.value()));
	} while (llvm::cast<llvm::ConstantInt>(
				 llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_NE, len,
													  llvm::ConstantInt::get(len->getType(), 0u, false)))
				 ->getValue()
				 .getBoolValue());
	String resStr;
	for (auto* dig : resultDigits) {
		resStr.append(std::to_string(*dig->getValue().getRawData()));
	}
	return resStr;
}

Maybe<bool> UnsignedType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (first->get_ir_type()->is_same(second->get_ir_type())) {
		return llvm::cast<llvm::ConstantInt>(
				   llvm::ConstantFoldConstant(llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ,
																				   first->get_llvm_constant(),
																				   second->get_llvm_constant()),
											  irCtx->dataLayout.value()))
			->getValue()
			.getBoolValue();
	} else {
		return false;
	}
}

} // namespace qat::ir
