#include "binary_expression.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/reference.hpp"
#include "operator.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#define QAT_COMPARISON_INDEX "qat'str'comparisonIndex"

namespace qat::ast {

IR::Value* BinaryExpression::emit(IR::Context* ctx) {
  IR::Value* lhsEmit = nullptr;
  IR::Value* rhsEmit = nullptr;
  if (lhs->nodeType() == NodeType::Default) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->isReference() ? rhsEmit->getType()->asReference()->getSubType()
                                                                     : rhsEmit->getType());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::Default) {
    lhsEmit = lhs->emit(ctx);
    rhs->asTypeInferrable()->setInferenceType(lhsEmit->isReference() ? lhsEmit->getType()->asReference()->getSubType()
                                                                     : lhsEmit->getType());
    rhsEmit = rhs->emit(ctx);
  } else if (lhs->nodeType() == NodeType::nullPointer) {
    rhsEmit = rhs->emit(ctx);
    if (rhsEmit->getType()->isPointer() ||
        (rhsEmit->getType()->isReference() && rhsEmit->getType()->asReference()->getSubType()->isPointer())) {
      lhs->asTypeInferrable()->setInferenceType(rhsEmit->isReference()
                                                    ? rhsEmit->getType()->asReference()->getSubType()->asPointer()
                                                    : rhsEmit->getType()->asPointer());
      lhsEmit = lhs->emit(ctx);
    } else {
      ctx->Error("Invalid type found to set for the null pointer. The LHS is a "
                 "null pointer and so RHS is expected to be of pointer type",
                 fileRange);
    }
  } else if (rhs->nodeType() == NodeType::nullPointer) {
    lhsEmit = lhs->emit(ctx);
    if (lhsEmit->getType()->isPointer() ||
        (lhsEmit->getType()->isReference() && lhsEmit->getType()->asReference()->getSubType()->isPointer())) {
      SHOW("Set type for RHS null pointer")
      rhs->asTypeInferrable()->setInferenceType(lhsEmit->isReference()
                                                    ? lhsEmit->getType()->asReference()->getSubType()->asPointer()
                                                    : lhsEmit->getType()->asPointer());
      rhsEmit = rhs->emit(ctx);
    } else {
      ctx->Error("Invalid type found to set for the null pointer. The LHS is a "
                 "null pointer and so RHS is expected to be of pointer type",
                 fileRange);
    }
  } else if ((lhs->nodeType() == NodeType::integerLiteral || lhs->nodeType() == NodeType::unsignedLiteral ||
              lhs->nodeType() == NodeType::floatLiteral || lhs->nodeType() == NodeType::customFloatLiteral ||
              lhs->nodeType() == NodeType::customIntegerLiteral) &&
             expectSymmetricOperandType(op)) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->getType());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->hasTypeInferrance() && expectSymmetricOperandType(op)) {
    lhsEmit = lhs->emit(ctx);
    auto lhsTy =
        lhsEmit->getType()->isReference() ? lhsEmit->getType()->asReference()->getSubType() : lhsEmit->getType();
    if (lhsTy->isCType()) {
      lhsTy = lhsTy->asCType()->getSubType();
    }
    if (lhsTy->isInteger() || lhsTy->isUnsignedInteger() || lhsTy->isFloat()) {
      rhs->asTypeInferrable()->setInferenceType(lhsEmit->getType());
    }
    rhsEmit = rhs->emit(ctx);
  } else {
    lhsEmit = lhs->emit(ctx);
    rhsEmit = rhs->emit(ctx);
  }

  SHOW("Operator is: " << OpToString(op))

  IR::QatType* lhsType          = lhsEmit->getType();
  IR::QatType* rhsType          = rhsEmit->getType();
  llvm::Value* lhsVal           = lhsEmit->getLLVM();
  llvm::Value* rhsVal           = rhsEmit->getLLVM();
  auto         referenceHandler = [&]() {
    SHOW("Reference handler ::")
    SHOW("Loading implicit LHS")
    lhsEmit->loadImplicitPointer(ctx->builder);
    SHOW("Loaded LHS")
    lhsVal = lhsEmit->getLLVM();
    SHOW("Loading implicit RHS")
    rhsEmit->loadImplicitPointer(ctx->builder);
    SHOW("Loaded RHS")
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
  auto lhsValueType = lhsType->isReference() ? lhsType->asReference()->getSubType() : lhsType;
  if (lhsValueType->isCType()) {
    lhsValueType = lhsValueType->asCType()->getSubType();
  }
  auto rhsValueType = rhsType->isReference() ? rhsType->asReference()->getSubType() : rhsType;
  if (rhsValueType->isCType()) {
    rhsValueType = rhsValueType->asCType()->getSubType();
  }
  if (lhsValueType->isInteger()) {
    referenceHandler();
    SHOW("Integer binary operation: " << OpToString(op))
    if (lhsValueType->isSame(rhsValueType)) {
      llvm::Value* llRes;
      IR::QatType* resType = lhsType;
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
          llRes   = ctx->builder.CreateICmpEQ(lhsVal, rhsVal);
          llRes   = ctx->builder.CreateIntCast(llRes, llvm::Type::getIntNTy(ctx->llctx, 1), false);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->builder.CreateICmpNE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->builder.CreateICmpSLT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->builder.CreateICmpSGT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->builder.CreateICmpSLE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->builder.CreateICmpSGE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = ctx->builder.CreateAnd(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseOr: {
          llRes = ctx->builder.CreateOr(lhsVal, rhsVal);
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
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::Or: {
          llRes   = ctx->builder.CreateOr(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
      }
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsValueType->isChoice() && rhsValueType->asChoice()->hasNegativeValues() &&
          (rhsValueType->asChoice()->getBitwidth() == lhsValueType->asInteger()->getBitwidth())) {
        if (op == Op::bitwiseAnd) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else if (op == Op::bitwiseOr) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else if (op == Op::bitwiseXor) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else {
          ctx->Error(" for left hand side type " + ctx->highlightError(lhsValueType->toString()) + " a", fileRange);
        }
      } else if (rhsValueType->isChoice()) {
        if (rhsValueType->asChoice()->hasNegativeValues()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->highlightError(std::to_string(lhsValueType->asInteger()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValueType->toString()) +
                         " whose underlying type is a signed integer with a bitwidth of " +
                         ctx->highlightError(std::to_string(rhsValueType->asChoice()->getBitwidth())),
                     fileRange);
        } else {
          ctx->Error(
              "The operand on the left is a signed integer of type " + ctx->highlightError(lhsValueType->toString()) +
                  " but the operand on the right is of the choice type " +
                  ctx->highlightError(rhsValueType->toString()) + " whose underlying type is an unsigned integer type",
              fileRange);
        }
      } else if (rhsValueType->isInteger()) {
        ctx->Error("Signed integers in this binary operation have different bitwidths."
                   " It is recommended to convert the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValueType->isUnsignedInteger()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "an unsigned integer. Please check logic or convert one side "
                   "to the other type if this was intentional",
                   fileRange);
      } else if (rhsValueType->isFloat()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "a floating point number. Please check logic or convert one "
                   "side to the other type if this was intentional",
                   fileRange);
      } else {
        // FIXME - Support side flipped operator
        ctx->Error("No operator found that matches both operand types. The left hand "
                   "side is " +
                       ctx->highlightError(lhsType->toString()) + ", and the right hand side is " +
                       ctx->highlightError(rhsType->toString()),
                   fileRange);
      }
    }
  } else if (lhsValueType->isUnsignedInteger()) {
    SHOW("Unsigned integer binary operation")
    referenceHandler();
    if (lhsType->isSame(rhsType)) {
      llvm::Value* llRes;
      IR::QatType* resType = lhsType;
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
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->builder.CreateICmpNE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->builder.CreateICmpULT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->builder.CreateICmpUGT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->builder.CreateICmpULE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->builder.CreateICmpUGE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = ctx->builder.CreateAnd(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseOr: {
          llRes = ctx->builder.CreateOr(lhsVal, rhsVal);
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
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::Or: {
          llRes   = ctx->builder.CreateOr(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsValueType->isChoice() && !rhsValueType->asChoice()->hasNegativeValues() &&
          (rhsValueType->asChoice()->getBitwidth() == lhsValueType->asInteger()->getBitwidth())) {
        if (op == Op::bitwiseAnd) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else if (op == Op::bitwiseOr) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else if (op == Op::bitwiseXor) {
          referenceHandler();
          return new IR::Value(ctx->builder.CreateXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValueType, false,
                               IR::Nature::temporary);
        } else {
          ctx->Error(" for left hand side type " + ctx->highlightError(lhsValueType->toString()) + " a", fileRange);
        }
      } else if (rhsValueType->isChoice()) {
        if (!rhsValueType->asChoice()->hasNegativeValues()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->highlightError(std::to_string(lhsValueType->asUnsignedInteger()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValueType->toString()) +
                         " whose underlying type is an unsigned integer with a bitwidth of " +
                         ctx->highlightError(std::to_string(rhsValueType->asChoice()->getBitwidth())),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is an unsigned integer of type " +
                         ctx->highlightError(lhsValueType->toString()) +
                         " but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValueType->toString()) +
                         " whose underlying type is a signed integer type",
                     fileRange);
        }
      } else if (rhsValueType->isUnsignedInteger()) {
        ctx->Error("Unsigned integers in this binary operation have different "
                   "bitwidths. Cast the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValueType->isInteger()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a signed integer. Please check logic or cast one side to the "
                   "other type if this was intentional",
                   fileRange);
      } else if (rhsValueType->isFloat()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a floating point number. Please check logic or cast one side to "
                   "the other type if this was intentional",
                   fileRange);
      } else {
        // FIXME - Support side flipped operator
        ctx->Error("No operator found that matches both operand types. The left hand "
                   "side is " +
                       ctx->highlightError(lhsType->toString()) + ", and the right hand side is " +
                       ctx->highlightError(rhsType->toString()),
                   fileRange);
      }
    }
  } else if (lhsValueType->isFloat()) {
    SHOW("Float binary operation")
    referenceHandler();
    if (lhsType->isSame(rhsType)) {
      llvm::Value* llRes   = nullptr;
      IR::QatType* resType = lhsType;
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
          ctx->getMod()->nativeLibsToLink.push_back(IR::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::remainder: {
          llRes = ctx->builder.CreateFRem(lhsVal, rhsVal);
          ctx->getMod()->nativeLibsToLink.push_back(IR::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::equalTo: {
          llRes   = ctx->builder.CreateFCmpOEQ(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->builder.CreateFCmpONE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->builder.CreateFCmpOLT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->builder.CreateFCmpOGT(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->builder.CreateFCmpOLE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->builder.CreateFCmpOGE(lhsVal, rhsVal);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = ctx->builder.CreateAnd(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseOr: {
          llRes = ctx->builder.CreateOr(lhsVal, rhsVal);
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
          ctx->Error("&& operator cannot have floating point numbers as operands", fileRange);
          return nullptr;
        }
        case Op::Or: {
          ctx->Error("|| operator cannot have floating point numbers as operands", fileRange);
          return nullptr;
        }
      }
      return new IR::Value(llRes, resType, false, IR::Nature::temporary);
    } else {
      if (rhsType->isFloat()) {
        ctx->Error(
            "Left hand side of the expression is " + lhsType->toString() + " and right hand side is " +
                rhsType->toString() +
                ". The floating point types do not match. Convert either value to the type of the other one if this was intentional",
            fileRange);
      } else if (rhsType->isInteger()) {
        ctx->Error("The right hand side of the expression is a signed integer. "
                   "If this was intentional, convert the integer value to " +
                       ctx->highlightError(lhsType->toString()),
                   fileRange);
      } else if (rhsType->isUnsignedInteger()) {
        ctx->Error("The right hand side of the expression is an unsigned integer. "
                   "If this was intentional, convert the unsigned integer value to " +
                       ctx->highlightError(lhsType->toString()),
                   fileRange);
      } else {
        ctx->Error("Left hand side of the expression is of type " + ctx->highlightError(lhsType->toString()) +
                       " and right hand side is of type " + rhsType->toString() + ". Please check the logic.",
                   fileRange);
      }
    }
  } else if (lhsValueType->isStringSlice() && rhsValueType->isStringSlice()) {
    if (op == Op::equalTo || op == Op::notEqualTo) {
      // NOLINTBEGIN(readability-isolate-declaration)
      llvm::Value *lhsBuff, *lhsCount, *rhsBuff, *rhsCount;
      bool         isConstantLHS = false, isConstantRHS = false;
      // NOLINTEND(readability-isolate-declaration)
      auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
      if (lhsEmit->isLLVMConstant()) {
        lhsBuff       = lhsEmit->getLLVMConstant()->getAggregateElement(0u);
        lhsCount      = lhsEmit->getLLVMConstant()->getAggregateElement(1u);
        isConstantLHS = true;
      } else {
        if (lhsType->isReference()) {
          lhsEmit->loadImplicitPointer(ctx->builder);
        } else if (!lhsEmit->isImplicitPointer()) {
          lhsEmit = lhsEmit->makeLocal(ctx, None, lhs->fileRange);
        }
        lhsBuff = ctx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(ctx->llctx),
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(), lhsEmit->getLLVM(), 0u));
        lhsCount =
            ctx->builder.CreateLoad(Ty64Int, ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(),
                                                                          lhsEmit->getLLVM(), 1u));
      }
      if (rhsEmit->isLLVMConstant()) {
        rhsBuff       = rhsEmit->getLLVMConstant()->getAggregateElement(0u);
        rhsCount      = rhsEmit->getLLVMConstant()->getAggregateElement(1u);
        isConstantRHS = true;
      } else {
        if (rhsType->isReference()) {
          rhsEmit->loadImplicitPointer(ctx->builder);
        } else if (!rhsEmit->isImplicitPointer()) {
          rhsEmit = rhsEmit->makeLocal(ctx, None, rhs->fileRange);
        }
        rhsBuff = ctx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(ctx->llctx),
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(), rhsEmit->getLLVM(), 0u));
        rhsCount =
            ctx->builder.CreateLoad(Ty64Int, ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(),
                                                                          rhsEmit->getLLVM(), 1u));
      }
      if (isConstantLHS && isConstantRHS) {
        SHOW("Both string slices are constant")
        auto strCmpRes = IR::Logic::compareConstantStrings(
            llvm::cast<llvm::Constant>(lhsBuff), llvm::cast<llvm::Constant>(lhsCount),
            llvm::cast<llvm::Constant>(rhsBuff), llvm::cast<llvm::Constant>(rhsCount), ctx->llctx);
        return new IR::PrerunValue(
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), (op == Op::equalTo) ? strCmpRes : !strCmpRes),
            IR::UnsignedType::getBool(ctx));
      }
      auto* curr              = ctx->getActiveFunction()->getBlock();
      auto* lenCheckTrueBlock = new IR::Block(ctx->getActiveFunction(), curr);
      auto* strCmpTrueBlock   = new IR::Block(ctx->getActiveFunction(), curr);
      auto* strCmpFalseBlock  = new IR::Block(ctx->getActiveFunction(), curr);
      auto* iterCondBlock     = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
      auto* iterTrueBlock     = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
      auto* iterIncrBlock     = new IR::Block(ctx->getActiveFunction(), iterTrueBlock);
      auto* iterFalseBlock    = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
      auto* restBlock         = new IR::Block(ctx->getActiveFunction(), curr->getParent());
      restBlock->linkPrevBlock(curr);
      auto* Ty8Int         = llvm::Type::getInt8Ty(ctx->llctx);
      auto* qatStrCmpIndex = ctx->getActiveFunction()->getFunctionCommonIndex();
      // NOTE - Length equality check
      ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(lhsCount, rhsCount), lenCheckTrueBlock->getBB(),
                                strCmpFalseBlock->getBB());
      //
      // NOTE - Length matches
      lenCheckTrueBlock->setActive(ctx->builder);
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u),
                               qatStrCmpIndex->getLLVM());
      (void)IR::addBranch(ctx->builder, iterCondBlock->getBB());
      //
      // NOTE - Checking the iteration count
      iterCondBlock->setActive(ctx->builder);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->getLLVM()), lhsCount),
          iterTrueBlock->getBB(), iterFalseBlock->getBB());
      //
      // NOTE - Iteration check is true
      iterTrueBlock->setActive(ctx->builder);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(
              ctx->builder.CreateLoad(
                  Ty8Int, ctx->builder.CreateInBoundsGEP(
                              Ty8Int, lhsBuff, {ctx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->getLLVM())})),
              ctx->builder.CreateLoad(
                  Ty8Int, ctx->builder.CreateInBoundsGEP(
                              Ty8Int, rhsBuff, {ctx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->getLLVM())}))),
          iterIncrBlock->getBB(), iterFalseBlock->getBB());
      //
      // NOTE - Increment string slice iteration count
      iterIncrBlock->setActive(ctx->builder);
      ctx->builder.CreateStore(ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->getLLVM()),
                                                      llvm::ConstantInt::get(Ty64Int, 1u)),
                               qatStrCmpIndex->getLLVM());
      (void)IR::addBranch(ctx->builder, iterCondBlock->getBB());
      //
      // NOTE - Iteration check is false, time to check if the count matches
      iterFalseBlock->setActive(ctx->builder);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(lhsCount, ctx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->getLLVM())),
          strCmpTrueBlock->getBB(), strCmpFalseBlock->getBB());
      //
      // NOTE - Merging branches to the resume block
      strCmpTrueBlock->setActive(ctx->builder);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      strCmpFalseBlock->setActive(ctx->builder);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      //
      // NOTE - Control flow resumes
      restBlock->setActive(ctx->builder);
      auto* Ty1Int    = llvm::Type::getInt1Ty(ctx->llctx);
      auto* strCmpRes = ctx->builder.CreatePHI(Ty1Int, 2);
      strCmpRes->addIncoming(llvm::ConstantInt::get(Ty1Int, (op == Op::equalTo) ? 1u : 0u), strCmpTrueBlock->getBB());
      strCmpRes->addIncoming(llvm::ConstantInt::get(Ty1Int, (op == Op::equalTo) ? 0u : 1u), strCmpFalseBlock->getBB());
      return new IR::Value(strCmpRes, IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
    } else {
      ctx->Error("String slice does not support the " + ctx->highlightError(OpToString(op)) + " operator", fileRange);
    }
  } else if (lhsValueType->isPointer() && rhsValueType->isPointer()) {
    SHOW("LHS type is: " << lhsType->toString() << " and RHS type is: " << rhsType->toString())
    if (lhsValueType->asPointer()->getSubType()->isSame(rhsValueType->asPointer()->getSubType()) &&
        (lhsValueType->asPointer()->isMulti() == rhsValueType->asPointer()->isMulti())) {
      if ((lhsEmit->isReference() ? (lhsEmit->getType()->asReference()->getSubType()->isPointer() &&
                                     lhsEmit->getType()->asReference()->getSubType()->asPointer()->isMulti())
                                  : (lhsEmit->getType()->isPointer() && lhsEmit->getType()->asPointer()->isMulti())) ||
          (rhsEmit->isReference() ? (rhsEmit->getType()->asReference()->getSubType()->isPointer() &&
                                     rhsEmit->getType()->asReference()->getSubType()->asPointer()->isMulti())
                                  : (rhsEmit->getType()->isPointer() && rhsEmit->getType()->asPointer()->isMulti()))) {
        if (lhsEmit->isReference() ? (lhsEmit->getType()->asReference()->getSubType()->isPointer() &&
                                      lhsEmit->getType()->asReference()->getSubType()->asPointer()->isMulti())
                                   : (lhsEmit->getType()->isPointer() && lhsEmit->getType()->asPointer()->isMulti())) {
          bool isLHSRef = false;
          SHOW("LHS side")
          if (lhsEmit->getType()->isReference()) {
            SHOW("Loading LHS")
            lhsEmit->loadImplicitPointer(ctx->builder);
            isLHSRef = true;
          } else if (lhsEmit->isImplicitPointer()) {
            isLHSRef = true;
          }
          SHOW("LHS: Getting pointer type")
          auto* ptrType = lhsEmit->getType()->isReference()
                              ? lhsEmit->getType()->asReference()->getSubType()->asPointer()
                              : lhsEmit->getType()->asPointer();
          SHOW("LHS got pointer type")
          lhsType       = ptrType;
          auto resPtrTy = IR::PointerType::get(false, ptrType->getSubType(), ptrType->isNonNullable(),
                                               IR::PointerOwner::OfAnonymous(), false, ctx);
          if (lhsEmit->isLLVMConstant()) {
            lhsEmit = new IR::PrerunValue(lhsEmit->getLLVMConstant()->getAggregateElement(0u), resPtrTy);
          } else if (isLHSRef) {
            lhsEmit = new IR::Value(
                ctx->builder.CreateLoad(resPtrTy->getLLVMType(),
                                        ctx->builder.CreateStructGEP(ptrType->getLLVMType(), lhsEmit->getLLVM(), 0u)),
                resPtrTy, false, IR::Nature::temporary);
          } else {
            lhsEmit = new IR::Value(ctx->builder.CreateExtractValue(lhsEmit->getLLVM(), {0u}), resPtrTy, false,
                                    IR::Nature::temporary);
          }
          lhsVal = lhsEmit->getLLVM();
          SHOW("Set LhsEmit")
        }
        if (rhsEmit->getType()->isReference()
                ? (rhsEmit->getType()->asReference()->getSubType()->isPointer() &&
                   rhsEmit->getType()->asReference()->getSubType()->asPointer()->isMulti())
                : (rhsEmit->getType()->isPointer() && rhsEmit->getType()->asPointer()->isMulti())) {
          bool isRHSRef = false;
          SHOW("RHS side")
          if (rhsEmit->getType()->isReference()) {
            SHOW("Loading RHS")
            rhsEmit->loadImplicitPointer(ctx->builder);
            isRHSRef = true;
          } else if (rhsEmit->isImplicitPointer()) {
            isRHSRef = true;
          }
          SHOW("RHS: Getting pointer type")
          auto* ptrType = rhsEmit->getType()->isReference()
                              ? rhsEmit->getType()->asReference()->getSubType()->asPointer()
                              : rhsEmit->getType()->asPointer();
          SHOW("RHS got pointer type")
          rhsType       = ptrType;
          auto resPtrTy = IR::PointerType::get(false, ptrType->getSubType(), ptrType->isNonNullable(),
                                               IR::PointerOwner::OfAnonymous(), false, ctx);
          if (rhsEmit->isLLVMConstant()) {
            rhsEmit = new IR::PrerunValue(rhsEmit->getLLVMConstant()->getAggregateElement(0u), resPtrTy);
          } else if (isRHSRef) {
            rhsEmit = new IR::Value(
                ctx->builder.CreateLoad(resPtrTy->getLLVMType(),
                                        ctx->builder.CreateStructGEP(ptrType->getLLVMType(), rhsEmit->getLLVM(), 0u)),
                resPtrTy, false, IR::Nature::temporary);
          } else {
            rhsEmit = new IR::Value(ctx->builder.CreateExtractValue(rhsEmit->getLLVM(), {0u}), resPtrTy, false,
                                    IR::Nature::temporary);
          }
          rhsVal = rhsEmit->getLLVM();
          SHOW("Set RhsEmit")
        }
      }
      SHOW("LHS type is: " << lhsType->toString() << " and RHS type is: " << rhsType->toString())
      auto ptrTy = lhsValueType->asPointer();
      if (op == Op::equalTo) {
        SHOW("Pointer is normal")
        return new IR::Value(
            ctx->builder.CreateICmpEQ(ctx->builder.CreatePtrDiff(ptrTy->getSubType()->getLLVMType(), lhsVal, rhsVal),
                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
            IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else if (op == Op::notEqualTo) {
        SHOW("Pointer is normal")
        return new IR::Value(
            ctx->builder.CreateICmpNE(ctx->builder.CreatePtrDiff(ptrTy->getSubType()->getLLVMType(), lhsVal, rhsVal),
                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
            IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else if (op == Op::subtract) {
        return new IR::Value(ctx->builder.CreatePtrDiff(ptrTy->getSubType()->getLLVMType(), lhsVal, rhsVal),
                             IR::CType::getPtrDiff(ctx), false, IR::Nature::temporary);
      } else {
        ctx->Error("The operands are pointers, and the operation " + ctx->highlightError(OpToString(op)) +
                       " is not supported for pointers",
                   fileRange);
      }
    } else {
      ctx->Error("The operands have different pointer types. The left hand side "
                 "is of type " +
                     ctx->highlightError(lhsType->toString()) + " and the right hand side is of type " +
                     ctx->highlightError(rhsType->toString()),
                 fileRange);
    }
  } else if (lhsType->isChoice() && rhsType->isSame(lhsType)) {
    referenceHandler();
    auto chTy = lhsType->asChoice();
    if (op == Op::equalTo) {
      return new IR::Value(ctx->builder.CreateICmpEQ(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                           IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
    } else if (op == Op::notEqualTo) {
      return new IR::Value(ctx->builder.CreateICmpNE(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                           IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
    } else if (op == Op::lessThan) {
      if (chTy->hasNegativeValues()) {
        return new IR::Value(ctx->builder.CreateICmpSLT(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else {
        new IR::Value(ctx->builder.CreateICmpULT(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                      IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else if (op == Op::lessThanOrEqualTo) {
      if (chTy->hasNegativeValues()) {
        return new IR::Value(ctx->builder.CreateICmpSLE(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else {
        new IR::Value(ctx->builder.CreateICmpULE(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                      IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else if (op == Op::greaterThan) {
      if (chTy->hasNegativeValues()) {
        return new IR::Value(ctx->builder.CreateICmpSGT(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else {
        new IR::Value(ctx->builder.CreateICmpUGT(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                      IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else if (op == Op::greaterThanEqualTo) {
      if (chTy->hasNegativeValues()) {
        return new IR::Value(ctx->builder.CreateICmpSGE(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      } else {
        new IR::Value(ctx->builder.CreateICmpUGE(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                      IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else {
      // FIXME - ?? Separate type for bitwise operations
      if (op == Op::bitwiseAnd) {
        return new IR::Value(ctx->builder.CreateAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             chTy->hasNegativeValues() ? (IR::QatType*)IR::IntegerType::get(chTy->getBitwidth(), ctx)
                                                       : IR::UnsignedType::get(chTy->getBitwidth(), ctx),
                             false, IR::Nature::temporary);
      } else if (op == Op::bitwiseOr) {
        return new IR::Value(ctx->builder.CreateOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             chTy->hasNegativeValues() ? (IR::QatType*)IR::IntegerType::get(chTy->getBitwidth(), ctx)
                                                       : IR::UnsignedType::get(chTy->getBitwidth(), ctx),
                             false, IR::Nature::temporary);
      } else if (op == Op::bitwiseXor) {
        return new IR::Value(ctx->builder.CreateXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             chTy->hasNegativeValues() ? (IR::QatType*)IR::IntegerType::get(chTy->getBitwidth(), ctx)
                                                       : IR::UnsignedType::get(chTy->getBitwidth(), ctx),
                             false, IR::Nature::temporary);
      } else {
        ctx->Error("This binary operator is not supported for expressions of type " +
                       ctx->highlightError(lhsType->toString()),
                   fileRange);
      }
    }
  } else if (lhsValueType->isChoice() &&
             (lhsValueType->asChoice()->hasNegativeValues() ? rhsValueType->isInteger()
                                                            : rhsValueType->isUnsignedInteger())) {
    auto lhsBits = lhsValueType->asChoice()->getBitwidth();
    auto rhsBits = rhsValueType->isUnsignedInteger() ? rhsValueType->asUnsignedInteger()->getBitwidth()
                                                     : rhsValueType->asInteger()->getBitwidth();
    if (lhsBits != rhsBits) {
      ctx->Error("The left hand side is of the choice type " + ctx->highlightError(lhsValueType->toString()) +
                     " whose underlying bitwidth is " + ctx->highlightError(std::to_string(lhsBits)) +
                     " but the right hand side has an " +
                     (rhsValueType->isUnsignedInteger() ? "unsigned integer" : "signed integer") +
                     " type of bitwidth " + ctx->highlightError(std::to_string(rhsBits)),
                 fileRange);
    } else {
      if (op == Op::bitwiseAnd) {
        return new IR::Value(ctx->builder.CreateAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             lhsValueType->asChoice()->hasNegativeValues()
                                 ? (IR::QatType*)IR::IntegerType::get(lhsBits, ctx)
                                 : IR::UnsignedType::get(lhsBits, ctx),
                             false, IR::Nature::temporary);
      } else if (op == Op::bitwiseOr) {
        return new IR::Value(ctx->builder.CreateOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             lhsValueType->asChoice()->hasNegativeValues()
                                 ? (IR::QatType*)IR::IntegerType::get(lhsBits, ctx)
                                 : IR::UnsignedType::get(lhsBits, ctx),
                             false, IR::Nature::temporary);
      } else if (op == Op::bitwiseXor) {
        return new IR::Value(ctx->builder.CreateXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                             lhsValueType->asChoice()->hasNegativeValues()
                                 ? (IR::QatType*)IR::IntegerType::get(lhsBits, ctx)
                                 : IR::UnsignedType::get(lhsBits, ctx),
                             false, IR::Nature::temporary);
      } else {
        ctx->Error("Left hand side is of choice type " + ctx->highlightError(lhsValueType->toString()) +
                       " and right hand side is of type " + ctx->highlightError(rhsValueType->toString()) +
                       ". Binary operator " + ctx->highlightError(OpToString(op)) +
                       " is not supported for these operands",
                   fileRange);
      }
    }
  } else {
    if (lhsType->isExpanded() || (lhsType->isReference() && lhsType->asReference()->getSubType()->isExpanded())) {
      SHOW("Expanded type binary operation")
      auto* eTy   = lhsType->isReference() ? lhsType->asReference()->getSubType()->asExpanded() : lhsType->asExpanded();
      auto  OpStr = OpToString(op);
      bool  isVarExp = lhsType->isReference() ? lhsType->asReference()->isSubtypeVariable()
                                              : (lhsEmit->isImplicitPointer() ? lhsEmit->isVariable() : true);
      if ((isVarExp &&
           eTy->hasVariationBinaryOperator(
               OpStr, {rhsEmit->isImplicitPointer() ? Maybe<bool>(rhsEmit->isVariable()) : None, rhsType})) ||
          eTy->hasNormalBinaryOperator(
              OpStr, {rhsEmit->isImplicitPointer() ? Maybe<bool>(rhsEmit->isVariable()) : None, rhsType})) {
        SHOW("RHS is matched exactly")
        auto localID = lhsEmit->getLocalID();
        if (!lhsType->isReference() && !lhsEmit->isImplicitPointer()) {
          lhsEmit = lhsEmit->makeLocal(ctx, None, lhs->fileRange);
        } else if (lhsType->isReference()) {
          lhsEmit->loadImplicitPointer(ctx->builder);
        }
        auto* opFn =
            (isVarExp &&
             eTy->hasVariationBinaryOperator(
                 OpStr, {rhsEmit->isImplicitPointer() ? Maybe<bool>(rhsEmit->isVariable()) : None, rhsType}))
                ? eTy->getVariationBinaryOperator(
                      OpStr, {lhsEmit->isImplicitPointer() ? Maybe<bool>(lhsEmit->isVariable()) : None, rhsType})
                : eTy->getNormalBinaryOperator(
                      OpStr, {lhsEmit->isImplicitPointer() ? Maybe<bool>(lhsEmit->isVariable()) : None, rhsType});
        if (!opFn->isAccessible(ctx->getAccessInfo())) {
          ctx->Error(String(isVarExp ? "Variation b" : "B") + "inary operator " + ctx->highlightError(OpToString(op)) +
                         " of type " + ctx->highlightError(eTy->getFullName()) + " with right hand side type " +
                         ctx->highlightError(rhsType->toString()) + " is not accessible here",
                     fileRange);
        }
        rhsEmit->loadImplicitPointer(ctx->builder);
        return opFn->call(ctx, {lhsEmit->getLLVM(), rhsEmit->getLLVM()}, localID, ctx->getMod());
      } else if (rhsType->isReference() &&
                 ((isVarExp && eTy->hasVariationBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()})) ||
                  eTy->hasNormalBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()}))) {
        auto localID = rhsEmit->getLocalID();
        rhsEmit->loadImplicitPointer(ctx->builder);
        SHOW("RHS is matched as subtype of reference")
        if (!lhsType->isReference() && !lhsEmit->isImplicitPointer()) {
          lhsEmit = lhsEmit->makeLocal(ctx, None, lhs->fileRange);
        }
        auto* opFn = (isVarExp && eTy->hasVariationBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()}))
                         ? eTy->getVariationBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()})
                         : eTy->getNormalBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()});
        if (!opFn->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("Operator " + ctx->highlightError(OpToString(op)) + " of type " +
                         ctx->highlightError(eTy->getFullName()) + " with right hand side type: " +
                         ctx->highlightError(rhsType->toString()) + " is not accessible here",
                     fileRange);
        }
        return opFn->call(
            ctx,
            {lhsEmit->getLLVM(),
             ctx->builder.CreateLoad(rhsType->asReference()->getSubType()->getLLVMType(), rhsEmit->getLLVM())},
            localID, ctx->getMod());
      } else {
        if (!isVarExp &&
            eTy->hasVariationBinaryOperator(
                OpStr, {rhsEmit->isImplicitPointer() ? Maybe<bool>(rhsEmit->isVariable()) : None, rhsType})) {
          ctx->Error("Binary Operator " + ctx->highlightError(OpStr) + " with right hand side of type " +
                         ctx->highlightError(rhsType->toString()) + " is a variation of type " +
                         ctx->highlightError(eTy->getFullName()) +
                         " but the expression on the left hand side does not have variability",
                     fileRange);
        } else if (rhsType->isReference() && !isVarExp &&
                   eTy->hasVariationBinaryOperator(OpStr, {None, rhsType->asReference()->getSubType()})) {
          ctx->Error("Binary Operator " + ctx->highlightError(OpStr) + " with right hand side of type " +
                         ctx->highlightError(rhsType->asReference()->getSubType()->toString()) +
                         " is a variation of type " + ctx->highlightError(eTy->getFullName()) +
                         " but the expression on the left hand side does not have variability",
                     fileRange);
        }
        ctx->Error("Matching binary operator " + ctx->highlightError(OpToString(op)) + " not found for type " +
                       ctx->highlightError(eTy->getFullName() + " with right hand side of type " +
                                           ctx->highlightError(rhsType->toString())),
                   fileRange);
      }
    } else {
      // FIXME - Implement
      ctx->Error("Invalid type for the left hand side of the operator " + ctx->highlightError(OpToString(op)) +
                     ". Left hand side is of type " + ctx->highlightError(lhsType->toString()) +
                     " and the right hand side is of type " + ctx->highlightError(rhsType->toString()),
                 fileRange);
    }
  }
}

Json BinaryExpression::toJson() const {
  return Json()
      ._("nodeType", "binaryExpression")
      ._("operator", OpToString(op))
      ._("lhs", lhs->toJson())
      ._("rhs", rhs->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
