#include "./unsigned.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

UnsignedType::UnsignedType(u64 _bitWidth, IR::Context* _ctx, bool _isBool)
    : ctx(_ctx), bitWidth(_bitWidth), isBool(_isBool) {
  llvmType    = llvm::IntegerType::get(ctx->llctx, bitWidth);
  linkingName = "qat'" + toString();
}

UnsignedType* UnsignedType::get(u64 bits, IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBitWidth(bits) && !typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(bits, ctx, false);
}

UnsignedType* UnsignedType::getBool(IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(1u, ctx, true);
}

u64 UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(u64 width) const { return bitWidth == width; }

bool UnsignedType::isBoolean() const { return isBool; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

bool UnsignedType::isTypeSized() const { return true; }

String UnsignedType::toString() const { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

bool UnsignedType::canBePrerunGeneric() const { return true; }

Maybe<String> UnsignedType::toPrerunGenericString(IR::PrerunValue* val) const {
  llvm::ConstantInt* digit = nullptr;
  auto               len   = llvm::ConstantInt::get(llvm::Type::getInt64Ty(getLLVMType()->getContext()), 1u, false);
  auto               value = val->getLLVMConstant();
  auto               temp  = value;
  while (
      llvm::cast<llvm::ConstantInt>(
          llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGE, temp,
                                                                 llvm::ConstantInt::get(value->getType(), 10u, false)),
                                     ctx->dataLayout.value()))
          ->getValue()
          .getBoolValue()) {
    len         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getAdd(len, llvm::ConstantInt::get(len->getType(), 1u, false)), ctx->dataLayout.value()));
    auto* radix = llvm::ConstantInt::get(temp->getType(), 10u, false);
    temp        = llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::UDiv,
        llvm::ConstantExpr::getSub(temp, llvm::ConstantFoldBinaryOpOperands(llvm::Instruction::BinaryOps::URem, temp,
                                                                                   radix, ctx->dataLayout.value())),
        radix, ctx->dataLayout.value());
  }
  Vec<llvm::ConstantInt*> resultDigits(*len->getValue().getRawData(), nullptr);
  do {
    digit                                       = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::URem, value, llvm::ConstantInt::get(value->getType(), 10u, false),
        ctx->dataLayout.value()));
    len                                         = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getSub(len, llvm::ConstantInt::get(len->getType(), 1u, false)), ctx->dataLayout.value()));
    resultDigits[*len->getValue().getRawData()] = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
        llvm::ConstantExpr::getIntegerCast(digit, llvm::Type::getInt8Ty(ctx->llctx), false), ctx->dataLayout.value()));
    value                                       = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldBinaryOpOperands(
        llvm::Instruction::BinaryOps::UDiv, llvm::ConstantExpr::getSub(value, digit),
        llvm::ConstantInt::get(value->getType(), 10u, false), ctx->dataLayout.value()));
  } while (llvm::cast<llvm::ConstantInt>(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, len,
                                                                     llvm::ConstantInt::get(len->getType(), 0u, false)))
               ->getValue()
               .getBoolValue());
  String resStr;
  for (auto* dig : resultDigits) {
    resStr.append(std::to_string(*dig->getValue().getRawData()));
  }
  return resStr;
}

Maybe<bool> UnsignedType::equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return llvm::cast<llvm::ConstantInt>(
               llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                                                      first->getLLVMConstant(),
                                                                      second->getLLVMConstant()),
                                          ctx->dataLayout.value()))
        ->getValue()
        .getBoolValue();
  } else {
    return false;
  }
}

} // namespace qat::IR