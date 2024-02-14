#include "./integer.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "qat_type.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"

#define MAXIMUM_CONST_EXPR_BITWIDTH 64u

namespace qat::IR {

IntegerType::IntegerType(u64 _bitWidth, IR::Context* _ctx) : bitWidth(_bitWidth), ctx(_ctx) {
  llvmType    = llvm::IntegerType::get(ctx->llctx, bitWidth);
  linkingName = "qat'" + toString();
}

IntegerType* IntegerType::get(u64 bits, IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isInteger()) {
      if (typ->asInteger()->isBitWidth(bits)) {
        return typ->asInteger();
      }
    }
  }
  return new IntegerType(bits, ctx);
}

bool IntegerType::isBitWidth(u64 width) const { return bitWidth == width; }

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

u64 IntegerType::getBitwidth() const { return bitWidth; }

bool IntegerType::isTypeSized() const { return true; }

String IntegerType::toString() const { return "i" + std::to_string(bitWidth); }

IR::PrerunValue* IntegerType::getPrerunDefaultValue(IR::Context* ctx) {
  return new IR::PrerunValue(llvm::ConstantInt::get(getLLVMType(), 0u, true), this);
}

Maybe<String> IntegerType::toPrerunGenericString(IR::PrerunValue* val) const {
  llvm::ConstantInt* digit = nullptr;
  auto               len   = llvm::ConstantInt::get(llvm::Type::getInt64Ty(getLLVMType()->getContext()), 1u, false);
  auto               isValNegative = llvm::cast<llvm::ConstantInt>(
                           llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT, val->getLLVMConstant(),
                                                                     llvm::ConstantInt::get(val->getType()->getLLVMType(), 0u, true)))
                           ->getValue()
                           .getBoolValue();
  auto value = val->getLLVMConstant();
  if (isValNegative) {
    value = llvm::cast<llvm::ConstantInt>(
        llvm::ConstantFoldConstant(llvm::ConstantExpr::getNeg(value), ctx->dataLayout.value()));
  }
  auto temp = value;
  while (
      llvm::cast<llvm::ConstantInt>(
          llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGE, temp,
                                                                 llvm::ConstantInt::get(value->getType(), 10u, true)),
                                     ctx->dataLayout.value()))
          ->getValue()
          .getBoolValue()) {
    len         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getAdd(len, llvm::ConstantInt::get(len->getType(), 1u, false)), ctx->dataLayout.value()));
    auto* radix = llvm::ConstantInt::get(temp->getType(), 10u, true);
    temp        = llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::SDiv,
        llvm::ConstantExpr::getSub(temp, llvm::ConstantFoldBinaryOpOperands(llvm::Instruction::BinaryOps::SRem, temp,
                                                                                   radix, ctx->dataLayout.value())),
        radix, ctx->dataLayout.value());
  }
  Vec<llvm::ConstantInt*> resultDigits(*len->getValue().getRawData(), nullptr);
  do {
    digit                                       = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::SRem, value, llvm::ConstantInt::get(value->getType(), 10u, true),
        ctx->dataLayout.value()));
    len                                         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getSub(len, llvm::ConstantInt::get(len->getType(), 1u, false)), ctx->dataLayout.value()));
    resultDigits[*len->getValue().getRawData()] = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getIntegerCast(digit, llvm::Type::getInt8Ty(ctx->llctx), false), ctx->dataLayout.value()));
    value                                       = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::SDiv, llvm::ConstantExpr::getSub(value, digit),
        llvm::ConstantInt::get(value->getType(), 10u, true), ctx->dataLayout.value()));
  } while (llvm::cast<llvm::ConstantInt>(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, len,
                                                                     llvm::ConstantInt::get(len->getType(), 0u, false)))
               ->getValue()
               .getBoolValue());
  String resStr = isValNegative ? "-" : "";
  for (auto* dig : resultDigits) {
    resStr.append(std::to_string(*dig->getValue().getRawData()));
  }
  return resStr;
}

Maybe<bool> IntegerType::equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return (
        llvm::cast<llvm::ConstantInt>(
            llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                                                   first->getLLVMConstant(), second->getLLVMConstant()),
                                       ctx->dataLayout.value()))
            ->getValue()
            .getBoolValue());
  } else {
    return false;
  }
}

} // namespace qat::IR