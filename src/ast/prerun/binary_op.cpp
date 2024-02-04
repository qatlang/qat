#include "./binary_op.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

IR::PrerunValue* PrerunBinaryOp::emit(IR::Context* ctx) {
  IR::PrerunValue* lhsEmit = nullptr;
  IR::PrerunValue* rhsEmit = nullptr;
  if (lhs->nodeType() == NodeType::DEFAULT || lhs->nodeType() == NodeType::NULL_POINTER) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->getType());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::DEFAULT || rhs->nodeType() == NodeType::NULL_POINTER) {
    lhsEmit = lhs->emit(ctx);
    rhs->asTypeInferrable()->setInferenceType(lhsEmit->getType());
    rhsEmit = rhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::NULL_POINTER) {
    lhsEmit = lhs->emit(ctx);
    rhs->asTypeInferrable()->setInferenceType(lhsEmit->getType());
    rhsEmit = rhs->emit(ctx);
  } else if ((lhs->nodeType() == NodeType::INTEGER_LITERAL || lhs->nodeType() == NodeType::UNSIGNED_LITERAL ||
              lhs->nodeType() == NodeType::FLOAT_LITERAL || lhs->nodeType() == NodeType::CUSTOM_FLOAT_LITERAL ||
              lhs->nodeType() == NodeType::CUSTOM_INTEGER_LITERAL) &&
             expect_same_operand_types(opr)) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->getType());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->hasTypeInferrance() && expect_same_operand_types(opr)) {
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
  auto lhsType  = lhsEmit->getType();
  auto rhsType  = rhsEmit->getType();
  auto lhsValTy = lhsType->isCType() ? lhsType->asCType()->getSubType() : lhsType;
  auto rhsValTy = rhsType->isCType() ? rhsType->asCType()->getSubType() : rhsType;
  if (lhsValTy->isInteger()) {
    if (lhsType->isSame(rhsType)) {
      auto            lhsConst = lhsEmit->getLLVMConstant();
      auto            rhsConst = rhsEmit->getLLVMConstant();
      IR::QatType*    resType  = lhsType;
      llvm::Constant* llRes    = nullptr;
      switch (opr) {
        case Op::add: {
          llRes = llvm::ConstantExpr::getAdd(lhsConst, rhsConst);
          break;
        }
        case Op::subtract: {
          llRes = llvm::ConstantExpr::getSub(lhsConst, rhsConst);
          break;
        }
        case Op::multiply: {
          llRes = llvm::ConstantExpr::getMul(lhsConst, rhsConst);
          break;
        }
        case Op::divide: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateSDiv(lhsConst, rhsConst));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateSRem(lhsConst, rhsConst));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = llvm::ConstantExpr::getAnd(lhsConst, rhsConst);
          break;
        }
        case Op::bitwiseOr: {
          llRes = llvm::ConstantExpr::getOr(lhsConst, rhsConst);
          break;
        }
        case Op::bitwiseXor: {
          llRes = llvm::ConstantExpr::getXor(lhsConst, rhsConst);
          break;
        }
        case Op::logicalLeftShift: {
          llRes = llvm::ConstantExpr::getShl(lhsConst, rhsConst);
          break;
        }
        case Op::logicalRightShift: {
          llRes = llvm::ConstantExpr::getLShr(lhsConst, rhsConst);
          break;
        }
        case Op::arithmeticRightShift: {
          llRes = llvm::ConstantExpr::getAShr(lhsConst, rhsConst);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->highlightError(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->highlightError(lhsType->toString()) +
                         " and " + ctx->highlightError(rhsType->toString()),
                     fileRange);
        }
      }
      return new IR::PrerunValue(llRes, resType);
    } else {
      if (rhsValTy->isChoice() && (rhsValTy->asChoice()->getUnderlyingType()->isSame(lhsValTy))) {
        if (opr == Op::bitwiseAnd) {
          return new IR::PrerunValue(llvm::ConstantExpr::getAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else if (opr == Op::bitwiseOr) {
          return new IR::PrerunValue(llvm::ConstantExpr::getOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else if (opr == Op::bitwiseXor) {
          return new IR::PrerunValue(llvm::ConstantExpr::getXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else {
          ctx->Error("Unsupported operator " + ctx->highlightError(operator_to_string(opr)) +
                         " for left hand side type " + ctx->highlightError(lhsValTy->toString()) +
                         " and right hand side type " + ctx->highlightError(rhsValTy->toString()),
                     fileRange);
        }
      } else if (rhsValTy->isChoice()) {
        if (rhsValTy->asChoice()->hasNegativeValues()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->highlightError(std::to_string(lhsValTy->asInteger()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValTy->toString()) + " whose underlying type is " +
                         ctx->highlightError(rhsValTy->asChoice()->getUnderlyingType()->toString()),
                     fileRange);
        } else {
          ctx->Error(
              "The operand on the left is a signed integer of type " + ctx->highlightError(lhsValTy->toString()) +
                  " but the operand on the right is of the choice type " + ctx->highlightError(rhsValTy->toString()) +
                  " whose underlying type is an unsigned integer type",
              fileRange);
        }
      } else if (rhsValTy->isInteger()) {
        ctx->Error("Signed integers in this binary operation have different bitwidths."
                   " It is recommended to convert the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValTy->isUnsignedInteger()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "an unsigned integer. Please check logic or convert one side "
                   "to the other type if this was intentional",
                   fileRange);
      } else if (rhsValTy->isFloat()) {
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
  } else if (lhsValTy->isUnsignedInteger()) {
    if (lhsType->isSame(rhsType)) {
      auto            lhsConst = lhsEmit->getLLVMConstant();
      auto            rhsConst = rhsEmit->getLLVMConstant();
      IR::QatType*    resType  = lhsType;
      llvm::Constant* llRes    = nullptr;
      switch (opr) {
        case Op::add: {
          llRes = llvm::ConstantExpr::getAdd(lhsConst, rhsConst);
          break;
        }
        case Op::subtract: {
          llRes = llvm::ConstantExpr::getSub(lhsConst, rhsConst);
          break;
        }
        case Op::multiply: {
          llRes = llvm::ConstantExpr::getMul(lhsConst, rhsConst);
          break;
        }
        case Op::divide: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateUDiv(lhsConst, rhsConst));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateURem(lhsConst, rhsConst));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = llvm::ConstantExpr::getAnd(lhsConst, rhsConst);
          break;
        }
        case Op::bitwiseOr: {
          llRes = llvm::ConstantExpr::getOr(lhsConst, rhsConst);
          break;
        }
        case Op::bitwiseXor: {
          llRes = llvm::ConstantExpr::getXor(lhsConst, rhsConst);
          break;
        }
        case Op::logicalLeftShift: {
          llRes = llvm::ConstantExpr::getShl(lhsConst, rhsConst);
          break;
        }
        case Op::logicalRightShift: {
          llRes = llvm::ConstantExpr::getLShr(lhsConst, rhsConst);
          break;
        }
        case Op::arithmeticRightShift: {
          llRes = llvm::ConstantExpr::getAShr(lhsConst, rhsConst);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->highlightError(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->highlightError(lhsType->toString()) +
                         " and " + ctx->highlightError(rhsType->toString()),
                     fileRange);
        }
      }
      return new IR::PrerunValue(llRes, resType);
    } else {
      if (rhsValTy->isChoice() && rhsValTy->asChoice()->getUnderlyingType()->isSame(lhsValTy)) {
        if (opr == Op::bitwiseAnd) {
          return new IR::PrerunValue(llvm::ConstantExpr::getAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else if (opr == Op::bitwiseOr) {
          return new IR::PrerunValue(llvm::ConstantExpr::getOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else if (opr == Op::bitwiseXor) {
          return new IR::PrerunValue(llvm::ConstantExpr::getXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()), lhsValTy);
        } else {
          ctx->Error("Unsupported operator " + ctx->highlightError(operator_to_string(opr)) +
                         " for left hand side type " + ctx->highlightError(lhsType->toString()) +
                         " and right hand side type " + ctx->highlightError(rhsType->toString()),
                     fileRange);
        }
      } else if (rhsValTy->isChoice()) {
        if (!rhsValTy->asChoice()->hasNegativeValues()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->highlightError(std::to_string(lhsValTy->asUnsignedInteger()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValTy->toString()) + " whose underlying type is " +
                         ctx->highlightError(rhsValTy->asChoice()->getUnderlyingType()->toString()),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is an unsigned integer of type " +
                         ctx->highlightError(lhsValTy->toString()) +
                         " but the operand on the right is of the choice type " +
                         ctx->highlightError(rhsValTy->toString()) + " whose underlying type is a signed integer type",
                     fileRange);
        }
      } else if (rhsValTy->isUnsignedInteger()) {
        ctx->Error("Unsigned integers in this binary operation have different "
                   "bitwidths. Cast the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValTy->isInteger()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a signed integer. Please check logic or cast one side to the "
                   "other type if this was intentional",
                   fileRange);
      } else if (rhsValTy->isFloat()) {
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
  } else if (lhsValTy->isFloat()) {
    auto            lhsConst = lhsEmit->getLLVMConstant();
    auto            rhsConst = rhsEmit->getLLVMConstant();
    llvm::Constant* llRes    = nullptr;
    IR::QatType*    resType  = lhsType;
    if (lhsType->isSame(rhsType)) {
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (opr) {
        case Op::add: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateFAdd(lhsConst, rhsConst));
          break;
        }
        case Op::subtract: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateFSub(lhsConst, rhsConst));
          break;
        }
        case Op::multiply: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateFMul(lhsConst, rhsConst));
          break;
        }
        case Op::divide: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateFDiv(lhsConst, rhsConst));
          // ctx->getMod()->nativeLibsToLink.push_back(IR::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->builder.CreateFRem(lhsConst, rhsConst));
          // ctx->getMod()->nativeLibsToLink.push_back(IR::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OEQ, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_ONE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OLT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OGT, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OLE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OGE, lhsConst, rhsConst);
          resType = IR::UnsignedType::getBool(ctx);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->highlightError(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->highlightError(lhsType->toString()) +
                         " and " + ctx->highlightError(rhsType->toString()),
                     fileRange);
        }
      }
      return new IR::PrerunValue(llRes, resType);
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
  } else if (lhsType->isStringSlice() && rhsType->isStringSlice()) {
    if (opr == Op::equalTo) {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), lhsEmit->isEqualTo(ctx, rhsEmit) ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (opr == Op::notEqualTo) {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), lhsEmit->isEqualTo(ctx, rhsEmit) ? 0u : 1u),
          IR::UnsignedType::getBool(ctx));
    } else {
      ctx->Error("Unsupported operator " + ctx->highlightError(operator_to_string(opr)) + " for operands of type " +
                     ctx->highlightError(lhsType->toString()),
                 fileRange);
    }
  } else if (lhsType->isChoice() && lhsType->isSame(rhsType)) {
    auto chTy     = lhsType->asChoice();
    auto lhsConst = lhsEmit->getLLVMConstant();
    auto rhsConst = rhsEmit->getLLVMConstant();
    if (opr == Op::equalTo) {
      return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst),
                                 IR::UnsignedType::getBool(ctx));
    } else if (opr == Op::notEqualTo) {
      return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst),
                                 IR::UnsignedType::getBool(ctx));
    } else if (opr == Op::lessThan) {
      if (chTy->hasNegativeValues()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst),
                                   IR::UnsignedType::getBool(ctx));
      } else {
        new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst),
                            IR::UnsignedType::getBool(ctx));
      }
    } else if (opr == Op::lessThanOrEqualTo) {
      if (chTy->hasNegativeValues()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst),
                                   IR::UnsignedType::getBool(ctx));
      } else {
        new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst),
                            IR::UnsignedType::getBool(ctx));
      }
    } else if (opr == Op::greaterThan) {
      if (chTy->hasNegativeValues()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst),
                                   IR::UnsignedType::getBool(ctx));
      } else {
        new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst),
                            IR::UnsignedType::getBool(ctx));
      }
    } else if (opr == Op::greaterThanEqualTo) {
      if (chTy->hasNegativeValues()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst),
                                   IR::UnsignedType::getBool(ctx));
      } else {
        new IR::PrerunValue(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst),
                            IR::UnsignedType::getBool(ctx));
      }
    } else {
      // FIXME - ?? Separate type for bitwise operations
      if (opr == Op::bitwiseAnd) {
        return new IR::PrerunValue(llvm::ConstantExpr::getAnd(lhsConst, rhsConst), chTy->getUnderlyingType());
      } else if (opr == Op::bitwiseOr) {
        return new IR::PrerunValue(llvm::ConstantExpr::getOr(lhsConst, rhsConst), chTy->getUnderlyingType());
      } else if (opr == Op::bitwiseXor) {
        return new IR::PrerunValue(llvm::ConstantExpr::getXor(lhsConst, rhsConst), chTy->getUnderlyingType());
      } else {
        ctx->Error("Binary operator " + ctx->highlightError(operator_to_string(opr)) +
                       " is not supported for expressions of type " + ctx->highlightError(lhsType->toString()),
                   fileRange);
      }
    }
  } else if (lhsType->isChoice() &&
             (lhsType->asChoice()->hasNegativeValues() ? rhsValTy->isInteger() : rhsValTy->isUnsignedInteger())) {
    if (opr == Op::bitwiseAnd) {
      return new IR::PrerunValue(llvm::ConstantExpr::getAnd(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                                 lhsType->asChoice()->getUnderlyingType());
    } else if (opr == Op::bitwiseOr) {
      return new IR::PrerunValue(llvm::ConstantExpr::getOr(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                                 lhsType->asChoice()->getUnderlyingType());
    } else if (opr == Op::bitwiseXor) {
      return new IR::PrerunValue(llvm::ConstantExpr::getXor(lhsEmit->getLLVM(), rhsEmit->getLLVM()),
                                 lhsType->asChoice()->getUnderlyingType());
    } else {
      ctx->Error("Left hand side is of choice type " + ctx->highlightError(lhsType->toString()) +
                     " and right hand side is of type " + ctx->highlightError(rhsType->toString()) +
                     ". Binary operator " + ctx->highlightError(operator_to_string(opr)) +
                     " is not supported for these operands",
                 fileRange);
    }
  } else if (lhsValTy->isPointer() && rhsValTy->isPointer()) {
    if (lhsValTy->asPointer()->getSubType()->isSame(rhsValTy->asPointer()->getSubType()) &&
        (lhsValTy->asPointer()->isMulti() == rhsValTy->asPointer()->isMulti())) {
      llvm::Constant* finalCondition = nullptr;
      if (opr != Op::equalTo && opr != Op::notEqualTo) {
        ctx->Error("Unsupported operator " + ctx->highlightError(operator_to_string(opr)) +
                       " for expressions of type " + ctx->highlightError(lhsType->toString()) + " and " +
                       ctx->highlightError(rhsType->toString()),
                   fileRange);
      }
      finalCondition = llvm::ConstantExpr::getICmp(
          opr == Op::equalTo ? llvm::CmpInst::Predicate::ICMP_EQ : llvm::CmpInst::Predicate::ICMP_NE,
          llvm::ConstantExpr::getSub(
              llvm::ConstantExpr::getPtrToInt(
                  lhsValTy->asPointer()->isMulti() ? lhsEmit->getLLVM()->getAggregateElement(0u) : lhsEmit->getLLVM(),
                  llvm::Type::getInt64Ty(ctx->llctx)),
              llvm::ConstantExpr::getPtrToInt(
                  lhsValTy->asPointer()->isMulti() ? rhsEmit->getLLVM()->getAggregateElement(0u) : rhsEmit->getLLVM(),
                  llvm::Type::getInt64Ty(ctx->llctx))),
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u));
      if (lhsValTy->asPointer()->isMulti()) {
        finalCondition = llvm::ConstantExpr::getAnd(
            llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsEmit->getLLVM()->getAggregateElement(1u),
                                        rhsEmit->getLLVM()->getAggregateElement(1u)),
            finalCondition);
      }
      return new IR::PrerunValue(llvm::ConstantFoldConstant(finalCondition, ctx->dataLayout.value()),
                                 IR::UnsignedType::getBool(ctx));
    } else {
      ctx->Error("The operands have different pointer types. The left hand side "
                 "is of type " +
                     ctx->highlightError(lhsType->toString()) + " and the right hand side is of type " +
                     ctx->highlightError(rhsType->toString()),
                 fileRange);
    }
  } else {
    ctx->Error("Unsupported operand types for operator " + ctx->highlightError(operator_to_string(opr)) +
                   ". The left hand side is of type " + ctx->highlightError(lhsType->toString()) +
                   " and the right hand side is of type " + ctx->highlightError(rhsType->toString()),
               fileRange);
  }
}
String PrerunBinaryOp::toString() const {
  return lhs->toString() + " " + operator_to_string(opr) + " " + rhs->toString();
}

Json PrerunBinaryOp::toJson() const {
  return Json()
      ._("nodeType", "prerunBinaryOp")
      ._("lhs", lhs->toJson())
      ._("operator", operator_to_string(opr))
      ._("rhs", rhs->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast