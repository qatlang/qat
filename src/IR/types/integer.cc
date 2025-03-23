#include "./integer.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "./qat_type.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>

#define MAXIMUM_CONST_EXPR_BITWIDTH 64u

namespace qat::ir {

IntegerType::IntegerType(u64 _bitWidth, ir::Ctx* _ctx) : bitWidth(_bitWidth), irCtx(_ctx) {
	llvmType    = llvm::IntegerType::get(irCtx->llctx, bitWidth);
	linkingName = "qat'" + to_string();
}

IntegerType* IntegerType::get(u64 bits, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_integer()) {
			if (typ->as_integer()->is_bitwidth(bits)) {
				return typ->as_integer();
			}
		}
	}
	return std::construct_at(OwnNormal(IntegerType), bits, irCtx);
}

String IntegerType::to_string() const { return "i" + std::to_string(bitWidth); }

ir::PrerunValue* IntegerType::get_prerun_default_value(ir::Ctx* irCtx) {
	return ir::PrerunValue::get(llvm::ConstantInt::get(get_llvm_type(), 0u, true), this);
}

Maybe<String> IntegerType::to_prerun_generic_string(ir::PrerunValue* val) const {
	llvm::ConstantInt* digit = nullptr;
	auto               len   = llvm::ConstantInt::get(llvm::Type::getInt64Ty(get_llvm_type()->getContext()), 1u, false);
	auto               isValNegative =
	    llvm::cast<llvm::ConstantInt>(
	        llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SLT, val->get_llvm_constant(),
	                                             llvm::ConstantInt::get(val->get_llvm_constant()->getType(), 0u, true)))
	        ->getValue()
	        .getBoolValue();
	auto value = val->get_llvm_constant();
	if (isValNegative) {
		value = llvm::cast<llvm::ConstantInt>(
		    llvm::ConstantFoldConstant(llvm::ConstantExpr::getNeg(value), irCtx->dataLayout));
	}
	auto temp = value;
	while (llvm::cast<llvm::ConstantInt>(
	           llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SGE, temp,
	                                                llvm::ConstantInt::get(value->getType(), 10u, true)))
	           ->getValue()
	           .getBoolValue()) {
		len         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
            llvm::ConstantExpr::getAdd(len, llvm::ConstantInt::get(len->getType(), 1u, false)), irCtx->dataLayout));
		auto* radix = llvm::ConstantInt::get(temp->getType(), 10u, true);
		temp        = llvm::ConstantFoldBinaryOpOperands(
            llvm::Instruction::BinaryOps::SDiv,
            llvm::ConstantExpr::getSub(temp, llvm::ConstantFoldBinaryOpOperands(llvm::Instruction::BinaryOps::SRem,
		                                                                               temp, radix, irCtx->dataLayout)),
            radix, irCtx->dataLayout);
	}
	Vec<llvm::ConstantInt*> resultDigits(*len->getValue().getRawData(), nullptr);
	do {
		digit = llvm::cast<llvm::ConstantInt>(
		    llvm::ConstantFoldBinaryOpOperands(llvm::Instruction::BinaryOps::SRem, value,
		                                       llvm::ConstantInt::get(value->getType(), 10u, true), irCtx->dataLayout));
		len                                         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
            llvm::ConstantExpr::getSub(len, llvm::ConstantInt::get(len->getType(), 1u, false)), irCtx->dataLayout));
		resultDigits[*len->getValue().getRawData()] = llvm::cast<llvm::ConstantInt>(
		    llvm::ConstantFoldIntegerCast(digit, llvm::Type::getInt8Ty(irCtx->llctx), false, irCtx->dataLayout));
		value = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
		    llvm::Instruction::BinaryOps::SDiv, llvm::ConstantExpr::getSub(value, digit),
		    llvm::ConstantInt::get(value->getType(), 10u, true), irCtx->dataLayout));
	} while (llvm::cast<llvm::ConstantInt>(
	             llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_NE, len,
	                                                  llvm::ConstantInt::get(len->getType(), 0u, false)))
	             ->getValue()
	             .getBoolValue());
	String resStr = isValNegative ? "-" : "";
	for (auto* dig : resultDigits) {
		resStr.append(std::to_string(*dig->getValue().getRawData()));
	}
	return resStr;
}

Maybe<bool> IntegerType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (first->get_ir_type()->is_same(second->get_ir_type())) {
		return (llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ,
		                                                                           first->get_llvm_constant(),
		                                                                           second->get_llvm_constant()))
		            ->getValue()
		            .getBoolValue());
	} else {
		return false;
	}
}

} // namespace qat::ir
