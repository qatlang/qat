#include "binary_expression.hpp"
#include "../../IR/types/reference.hpp"
#include "operator.hpp"

namespace qat::ast {

IR::Value *BinaryExpression::emit(IR::Context *ctx) {
  auto *lhsEmit = lhs->emit(ctx);
  auto *rhsEmit = rhs->emit(ctx);

  SHOW("Operator is: " << OpToString(op))

  IR::QatType *lhsType          = lhsEmit->getType();
  IR::QatType *rhsType          = rhsEmit->getType();
  llvm::Value *lhsVal           = lhsEmit->getLLVM();
  llvm::Value *rhsVal           = rhsEmit->getLLVM();
  auto         referenceHandler = [&]() {
    lhsEmit->loadImplicitPointer(ctx->builder);
    lhsVal = lhsEmit->getLLVM();
    rhsEmit->loadImplicitPointer(ctx->builder);
    rhsVal = rhsEmit->getLLVM();
    if (lhsEmit->isReference()) {
      SHOW("LHS is reference")
      lhsType = lhsType->asReference()->getSubType();
      SHOW("LHS type is: " << lhsType->toString())
      lhsVal = ctx->builder.CreateLoad(lhsType->getLLVMType(), lhsVal, false);
    }
    if (rhsEmit->isReference()) {
      SHOW("RHS is reference")
      rhsType = rhsType->asReference()->getSubType();
      SHOW("RHS type is: " << rhsType->toString())
      rhsVal = ctx->builder.CreateLoad(rhsType->getLLVMType(), rhsVal, false);
    }
  };
  if (lhsType->isInteger() ||
      (lhsType->isReference() &&
       lhsType->asReference()->getSubType()->isInteger())) {
    referenceHandler();
    SHOW("Integer binary operation: " << OpToString(op))
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes;
      IR::QatType *resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (op) {
      case Op::add: {
        SHOW("Integer addition")
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
        llRes   = ctx->builder.CreateICmpNE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThan: {
        llRes   = ctx->builder.CreateICmpSLT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThan: {
        llRes   = ctx->builder.CreateICmpSGT(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::lessThanOrEqualTo: {
        llRes   = ctx->builder.CreateICmpSLE(lhsVal, rhsVal);
        resType = IR::UnsignedType::get(1, ctx->llctx);
        break;
      }
      case Op::greaterThanEqualTo: {
        llRes   = ctx->builder.CreateICmpSGE(lhsVal, rhsVal);
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
        // FIXME - Support side flipped operator
        ctx->Error(
            "No operator found that matches both operand types. The left hand "
            "side is a signed integer, and the right hand side is " +
                rhsType->toString(),
            fileRange);
      }
    }
  } else if (lhsType->isUnsignedInteger() ||
             (lhsType->isReference() &&
              lhsType->asReference()->getSubType()->isUnsignedInteger())) {
    SHOW("Unsigned integer binary operation")
    referenceHandler();
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes;
      IR::QatType *resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
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
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
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
        // FIXME - Support side flipped operator
        ctx->Error(
            "No operator found that matches both operand types. The left hand "
            "side is an unsigned integer, and the right hand side is " +
                rhsType->toString(),
            fileRange);
      }
    }
  } else if (lhsType->isFloat() ||
             (lhsType->isReference() &&
              lhsType->asReference()->getSubType()->isFloat())) {
    SHOW("Float binary operation")
    referenceHandler();
    if (lhsType->isSame(rhsType)) {
      llvm::Value *llRes   = nullptr;
      IR::QatType *resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
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
        // FIXME - Support flipped operator side
        ctx->Error("Left hand side of the expression is of type " +
                       ctx->highlightError(lhsType->toString()) +
                       " and right hand side is of type " +
                       rhsType->toString() + ". Please check the logic.",
                   fileRange);
      }
    }
  } else {
    if (lhsType->isCoreType() ||
        (lhsType->isReference() &&
         lhsType->asReference()->getSubType()->isCoreType())) {
      SHOW("Core type binary operation")
      auto *cTy   = lhsType->isReference()
                        ? lhsType->asReference()->getSubType()->asCore()
                        : lhsType->asCore();
      auto  OpStr = OpToString(op);
      // FIXME - Incomplete logic?
      if (cTy->hasBinaryOperator(OpStr, rhsType)) {
        SHOW("RHS is matched exactly")
        if (!lhsType->isReference() && !lhsEmit->isImplicitPointer()) {
          lhsEmit = lhsEmit->createAlloca(ctx->builder);
        }
        auto *opFn = cTy->getBinaryOperator(OpStr, rhsType);
        if (!opFn->isAccessible(ctx->getReqInfo())) {
          ctx->Error(
              "Binary operator " + ctx->highlightError(OpToString(op)) +
                  " of core type " + ctx->highlightError(cTy->getFullName()) +
                  " with RHS type " + ctx->highlightError(rhsType->toString()) +
                  " is not accessible here",
              fileRange);
        }
        rhsEmit->loadImplicitPointer(ctx->builder);
        return opFn->call(ctx, {lhsEmit->getLLVM(), rhsEmit->getLLVM()},
                          ctx->getMod());
      } else if (rhsType->isReference() &&
                 cTy->hasBinaryOperator(OpStr,
                                        rhsType->asReference()->getSubType())) {
        SHOW("RHS is matched as subtype of reference")
        if (!lhsType->isReference() && !lhsEmit->isImplicitPointer()) {
          lhsEmit = lhsEmit->createAlloca(ctx->builder);
        }
        auto *opFn =
            cTy->getBinaryOperator(OpStr, rhsType->asReference()->getSubType());
        if (!opFn->isAccessible(ctx->getReqInfo())) {
          ctx->Error("Operator " + ctx->highlightError(OpToString(op)) +
                         " of core type " +
                         ctx->highlightError(cTy->getFullName()) +
                         " with RHS type: " +
                         ctx->highlightError(rhsType->toString()) +
                         " is not accessible here",
                     fileRange);
        }
        return opFn->call(
            ctx,
            {lhsEmit->getLLVM(),
             ctx->builder.CreateLoad(
                 rhsType->asReference()->getSubType()->getLLVMType(),
                 rhsEmit->getLLVM())},
            ctx->getMod());
      } else {
        ctx->Error("Binary operator " + ctx->highlightError(OpToString(op)) +
                       " not defined for core type: " +
                       ctx->highlightError(
                           cTy->getFullName() + " that has RHS type: " +
                           ctx->highlightError(rhsType->toString())),
                   fileRange);
      }
    } else {
      // TODO - Implement
      ctx->Error("Invalid type of LHS for the operator " +
                     ctx->highlightError(OpToString(op)) + ". LHS is " +
                     ctx->highlightError(lhsType->toString()) + " and RHS is " +
                     ctx->highlightError(rhsType->toString()),
                 fileRange);
    }
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