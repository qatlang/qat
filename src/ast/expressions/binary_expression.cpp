#include "./binary_expression.hpp"
#include "../../IR/types/reference.hpp"
#include "operator.hpp"

namespace qat::ast {

IR::Value *BinaryExpression::emit(IR::Context *ctx) {
  auto *lhsEmit = lhs->emit(ctx);
  auto *rhsEmit = rhs->emit(ctx);

  // FIXME - Change this when introducing operators for core types
  lhsEmit->loadImplicitPointer(ctx->builder);
  rhsEmit->loadImplicitPointer(ctx->builder);

  IR::QatType *lhsType = lhsEmit->getType();
  IR::QatType *rhsType = rhsEmit->getType();
  llvm::Value *lhsVal  = lhsEmit->getLLVM();
  llvm::Value *rhsVal  = rhsEmit->getLLVM();
  if (lhsEmit->isReference()) {
    auto *subTy = lhsType->asReference()->getSubType();
    if (subTy->isInteger() || subTy->isFloat() || subTy->isUnsignedInteger()) {
      lhsType = lhsType->asReference()->getSubType();
      lhsVal  = ctx->builder.CreateLoad(lhsType->getLLVMType(), lhsVal, false);
    }
  }
  if (rhsEmit->isReference()) {
    auto *subTy = rhsType->asReference()->getSubType();
    if (subTy->isInteger() || subTy->isFloat() || subTy->isUnsignedInteger()) {
      rhsType = rhsType->asReference()->getSubType();
      rhsVal  = ctx->builder.CreateLoad(rhsType->getLLVMType(), rhsVal, false);
    }
  }
  if (lhsType->isInteger()) {
    SHOW("Integer binary operation: " << OpToString(op))
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes;
      IR::QatType *resType = lhsType;
      switch (op) {
      case Op::add: {
        llRes = ctx->builder.CreateAdd(lhsVal, rhsVal);
        break;
      }
      case Op::subtract: {
        llRes = ctx->builder.CreateSub(lhsVal, rhsVal);
        break;
      }
      case Op::multiply: {
        llRes = ctx->builder.CreateMul(lhsVal, rhsVal);
        break;
      }
      case Op::divide: {
        llRes = ctx->builder.CreateSDiv(lhsVal, rhsVal);
        break;
      }
      case Op::remainder: {
        llRes = ctx->builder.CreateSRem(lhsVal, rhsVal);
        break;
      }
      case Op::equalTo: {
        llRes = ctx->builder.CreateICmpEQ(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::notEqualTo: {
        llRes = ctx->builder.CreateICmpNE(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThan: {
        llRes = ctx->builder.CreateICmpSLT(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThan: {
        llRes = ctx->builder.CreateICmpSGT(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThanOrEqualTo: {
        llRes = ctx->builder.CreateICmpSLE(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThanEqualTo: {
        llRes = ctx->builder.CreateICmpSGE(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::bitwiseAnd: {
        llRes = ctx->builder.CreateLogicalAnd(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseOr: {
        llRes = ctx->builder.CreateLogicalOr(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseXor: {
        llRes = ctx->builder.CreateXor(lhsVal, rhsVal);
        break;
      }
      case Op::logicalLeftShift: {
        llRes = ctx->builder.CreateShl(lhsVal, rhsVal);
        break;
      }
      case Op::logicalRightShift: {
        llRes = ctx->builder.CreateLShr(lhsVal, rhsVal);
        break;
      }
      case Op::arithmeticRightShift: {
        llRes = ctx->builder.CreateAShr(lhsVal, rhsVal);
        break;
      }
      case Op::And: {
        llRes = ctx->builder.CreateAnd(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::Or: {
        llRes = ctx->builder.CreateOr(lhsVal, rhsVal);
        llRes = ctx->builder.CreateIntCast(
            llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      }
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsType->isInteger()) {
        ctx->Error(
            "Signed integers in this binary operation have different "
            "bitwidths. Cast the operand with smaller bitwidth to the bigger "
            "bitwidth to prevent potential loss of data and logical errors",
            fileRange);
      } else if (rhsType->isUnsignedInteger()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "an unsigned integer. Please check logic or cast one side "
                   "to the other type if this was intentional",
                   fileRange);
      } else if (rhsType->isFloat()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "a floating point number. Please check logic or cast one "
                   "side to the other type if this was intentional",
                   fileRange);
      } else {
        ctx->Error(
            "No operator found that matches both operand types. The left hand "
            "side is a signed integer, and the right hand side is " +
                rhsType->toString(),
            fileRange);
      }
    }
  } else if (lhsType->isUnsignedInteger()) {
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes;
      IR::QatType *resType = lhsType;
      switch (op) {
      case Op::add: {
        llRes = ctx->builder.CreateAdd(lhsVal, rhsVal);
        break;
      }
      case Op::subtract: {
        llRes = ctx->builder.CreateSub(lhsVal, rhsVal);
        break;
      }
      case Op::multiply: {
        llRes = ctx->builder.CreateMul(lhsVal, rhsVal);
        break;
      }
      case Op::divide: {
        llRes = ctx->builder.CreateUDiv(lhsVal, rhsVal);
        break;
      }
      case Op::remainder: {
        llRes = ctx->builder.CreateURem(lhsVal, rhsVal);
        break;
      }
      case Op::equalTo: {
        llRes   = ctx->builder.CreateICmpEQ(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::notEqualTo: {
        llRes   = ctx->builder.CreateICmpNE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThan: {
        llRes   = ctx->builder.CreateICmpULT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThan: {
        llRes   = ctx->builder.CreateICmpUGT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThanOrEqualTo: {
        llRes   = ctx->builder.CreateICmpULE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThanEqualTo: {
        llRes   = ctx->builder.CreateICmpUGE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::bitwiseAnd: {
        llRes = ctx->builder.CreateLogicalAnd(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseOr: {
        llRes = ctx->builder.CreateLogicalOr(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseXor: {
        llRes = ctx->builder.CreateXor(lhsVal, rhsVal);
        break;
      }
      case Op::logicalLeftShift: {
        llRes = ctx->builder.CreateShl(lhsVal, rhsVal);
        break;
      }
      case Op::logicalRightShift: {
        llRes = ctx->builder.CreateLShr(lhsVal, rhsVal);
        break;
      }
      case Op::arithmeticRightShift: {
        llRes = ctx->builder.CreateAShr(lhsVal, rhsVal);
        break;
      }
      case Op::And: {
        llRes   = ctx->builder.CreateAnd(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::Or: {
        llRes   = ctx->builder.CreateOr(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      }
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsType->isUnsignedInteger()) {
        ctx->Error(
            "Unsigned integers in this binary operation have different "
            "bitwidths. Cast the operand with smaller bitwidth to the bigger "
            "bitwidth to prevent potential loss of data and logical errors",
            fileRange);
      } else if (rhsType->isInteger()) {
        ctx->Error(
            "Left hand side is an unsigned integer and right hand side is "
            "a signed integer. Please check logic or cast one side to the "
            "other type if this was intentional",
            fileRange);
      } else if (rhsType->isFloat()) {
        ctx->Error(
            "Left hand side is an unsigned integer and right hand side is "
            "a floating point number. Please check logic or cast one side to "
            "the other type if this was intentional",
            fileRange);
      } else {
        ctx->Error(
            "No operator found that matches both operand types. The left hand "
            "side is an unsigned integer, and the right hand side is " +
                rhsType->toString(),
            fileRange);
      }
    }
  } else if (lhsType->isFloat()) {
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes   = nullptr;
      IR::QatType *resType = lhsType;
      switch (op) {
      case Op::add: {
        llRes = ctx->builder.CreateFAdd(lhsVal, rhsVal);
        break;
      }
      case Op::subtract: {
        llRes = ctx->builder.CreateFSub(lhsVal, rhsVal);
        break;
      }
      case Op::multiply: {
        llRes = ctx->builder.CreateFMul(lhsVal, rhsVal);
        break;
      }
      case Op::divide: {
        llRes = ctx->builder.CreateFDiv(lhsVal, rhsVal);
        break;
      }
      case Op::remainder: {
        llRes = ctx->builder.CreateFRem(lhsVal, rhsVal);
        break;
      }
      case Op::equalTo: {
        llRes   = ctx->builder.CreateFCmpOEQ(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::notEqualTo: {
        llRes   = ctx->builder.CreateFCmpONE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThan: {
        llRes   = ctx->builder.CreateFCmpOLT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThan: {
        llRes   = ctx->builder.CreateFCmpOGT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThanOrEqualTo: {
        llRes   = ctx->builder.CreateFCmpOLE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThanEqualTo: {
        llRes   = ctx->builder.CreateFCmpOGE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::bitwiseAnd: {
        llRes = ctx->builder.CreateLogicalAnd(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseOr: {
        llRes = ctx->builder.CreateLogicalOr(lhsVal, rhsVal);
        break;
      }
      case Op::bitwiseXor: {
        llRes = ctx->builder.CreateXor(lhsVal, rhsVal);
        break;
      }
      case Op::logicalLeftShift: {
        llRes = ctx->builder.CreateShl(lhsVal, rhsVal);
        break;
      }
      case Op::logicalRightShift: {
        llRes = ctx->builder.CreateLShr(lhsVal, rhsVal);
        break;
      }
      case Op::arithmeticRightShift: {
        llRes = ctx->builder.CreateAShr(lhsVal, rhsVal);
        break;
      }
      case Op::And: {
        ctx->Error("&& operator cannot have floating point numbers as operands",
                   fileRange);
        return nullptr;
      }
      case Op::Or: {
        ctx->Error("|| operator cannot have floating point numbers as operands",
                   fileRange);
        return nullptr;
      }
      }
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsType->isFloat()) {
        ctx->Error("Left hand side of the expression is " +
                       lhsType->toString() + " and right hand side is " +
                       rhsType->toString() +
                       ". Please check logic, or cast one of the values if "
                       "this was intentional",
                   fileRange);
      } else if (rhsType->isInteger()) {
        ctx->Error("The right hand side of the expression is a signed integer. "
                   "Cast that value if this was intentional, or check logic",
                   fileRange);
      } else if (rhsType->isUnsignedInteger()) {
        ctx->Error(
            "The right hand side of the expression is an unsigned integer. "
            "Cast that value if this was intentional, or check logic",
            fileRange);
      } else {
        ctx->Error("" + rhsType->toString(), fileRange);
      }
    }
  } else {
    // TODO - Implement
  }
}

nuo::Json BinaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "binaryExpression")
      ._("operator", OpToString(op))
      ._("lhs", lhs->toJson())
      ._("rhs", rhs->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast