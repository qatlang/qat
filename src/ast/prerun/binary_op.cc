#include "./binary_op.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

ir::PrerunValue* PrerunBinaryOp::emit(EmitCtx* ctx) {
  ir::PrerunValue* lhsEmit = nullptr;
  ir::PrerunValue* rhsEmit = nullptr;
  if (lhs->nodeType() == NodeType::DEFAULT || lhs->nodeType() == NodeType::NULL_POINTER) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->get_ir_type());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::DEFAULT || rhs->nodeType() == NodeType::NULL_POINTER) {
    lhsEmit = lhs->emit(ctx);
    rhs->asTypeInferrable()->setInferenceType(lhsEmit->get_ir_type());
    rhsEmit = rhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::NULL_POINTER) {
    lhsEmit = lhs->emit(ctx);
    rhs->asTypeInferrable()->setInferenceType(lhsEmit->get_ir_type());
    rhsEmit = rhs->emit(ctx);
  } else if ((lhs->nodeType() == NodeType::INTEGER_LITERAL || lhs->nodeType() == NodeType::UNSIGNED_LITERAL ||
              lhs->nodeType() == NodeType::FLOAT_LITERAL || lhs->nodeType() == NodeType::CUSTOM_FLOAT_LITERAL ||
              lhs->nodeType() == NodeType::CUSTOM_INTEGER_LITERAL) &&
             expect_same_operand_types(opr)) {
    rhsEmit = rhs->emit(ctx);
    lhs->asTypeInferrable()->setInferenceType(rhsEmit->get_ir_type());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->hasTypeInferrance() && expect_same_operand_types(opr)) {
    lhsEmit    = lhs->emit(ctx);
    auto lhsTy = lhsEmit->get_ir_type()->is_reference() ? lhsEmit->get_ir_type()->as_reference()->get_subtype()
                                                        : lhsEmit->get_ir_type();
    if (lhsTy->is_ctype()) {
      lhsTy = lhsTy->as_ctype()->get_subtype();
    }
    if (lhsTy->is_integer() || lhsTy->is_unsigned_integer() || lhsTy->is_float()) {
      rhs->asTypeInferrable()->setInferenceType(lhsEmit->get_ir_type());
    }
    rhsEmit = rhs->emit(ctx);
  } else {
    lhsEmit = lhs->emit(ctx);
    rhsEmit = rhs->emit(ctx);
  }
  auto lhsType  = lhsEmit->get_ir_type();
  auto rhsType  = rhsEmit->get_ir_type();
  auto lhsValTy = lhsType->is_ctype() ? lhsType->as_ctype()->get_subtype() : lhsType;
  auto rhsValTy = rhsType->is_ctype() ? rhsType->as_ctype()->get_subtype() : rhsType;
  if (lhsValTy->is_integer()) {
    if (lhsType->is_same(rhsType)) {
      auto            lhsConst = lhsEmit->get_llvm_constant();
      auto            rhsConst = rhsEmit->get_llvm_constant();
      ir::Type*       resType  = lhsType;
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
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateSDiv(lhsConst, rhsConst));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateSRem(lhsConst, rhsConst));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
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
          ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->color(lhsType->to_string()) + " and " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      }
      return ir::PrerunValue::get(llvm::ConstantFoldConstant(llRes, ctx->irCtx->dataLayout.value()), resType);
    } else {
      if (rhsValTy->is_choice() && (rhsValTy->as_choice()->get_underlying_type()->is_same(lhsValTy))) {
        if (opr == Op::bitwiseAnd) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getAnd(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else if (opr == Op::bitwiseOr) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getOr(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else if (opr == Op::bitwiseXor) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getXor(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else {
          ctx->Error("Unsupported operator " + ctx->color(operator_to_string(opr)) + " for left hand side type " +
                         ctx->color(lhsValTy->to_string()) + " and right hand side type " +
                         ctx->color(rhsValTy->to_string()),
                     fileRange);
        }
      } else if (rhsValTy->is_choice()) {
        if (rhsValTy->as_choice()->has_negative_values()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->color(std::to_string(lhsValTy->as_integer()->get_bitwidth())) +
                         ", but the operand on the right is of the choice type " + ctx->color(rhsValTy->to_string()) +
                         " whose underlying type is " +
                         ctx->color(rhsValTy->as_choice()->get_underlying_type()->to_string()),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is a signed integer of type " + ctx->color(lhsValTy->to_string()) +
                         " but the operand on the right is of the choice type " + ctx->color(rhsValTy->to_string()) +
                         " whose underlying type is an unsigned integer type",
                     fileRange);
        }
      } else if (rhsValTy->is_integer()) {
        ctx->Error("Signed integers in this binary operation have different bitwidths."
                   " It is recommended to convert the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValTy->is_unsigned_integer()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "an unsigned integer. Please check logic or convert one side "
                   "to the other type if this was intentional",
                   fileRange);
      } else if (rhsValTy->is_float()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "a floating point number. Please check logic or convert one "
                   "side to the other type if this was intentional",
                   fileRange);
      } else {
        // FIXME - ?? Support side flipped operator
        ctx->Error("No operator found that matches both operand types. The left hand "
                   "side is " +
                       ctx->color(lhsType->to_string()) + ", and the right hand side is " +
                       ctx->color(rhsType->to_string()),
                   fileRange);
      }
    }
  } else if (lhsValTy->is_unsigned_integer()) {
    if (lhsType->is_same(rhsType)) {
      auto            lhsConst = lhsEmit->get_llvm_constant();
      auto            rhsConst = rhsEmit->get_llvm_constant();
      ir::Type*       resType  = lhsType;
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
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateUDiv(lhsConst, rhsConst));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateURem(lhsConst, rhsConst));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
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
          ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->color(lhsType->to_string()) + " and " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      }
      return ir::PrerunValue::get(llvm::ConstantFoldConstant(llRes, ctx->irCtx->dataLayout.value()), resType);
    } else {
      if (rhsValTy->is_choice() && rhsValTy->as_choice()->get_underlying_type()->is_same(lhsValTy)) {
        if (opr == Op::bitwiseAnd) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getAnd(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else if (opr == Op::bitwiseOr) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getOr(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else if (opr == Op::bitwiseXor) {
          return ir::PrerunValue::get(llvm::ConstantExpr::getXor(lhsEmit->get_llvm(), rhsEmit->get_llvm()), lhsValTy);
        } else {
          ctx->Error("Unsupported operator " + ctx->color(operator_to_string(opr)) + " for left hand side type " +
                         ctx->color(lhsType->to_string()) + " and right hand side type " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      } else if (rhsValTy->is_choice()) {
        if (!rhsValTy->as_choice()->has_negative_values()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->color(std::to_string(lhsValTy->as_unsigned_integer()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " + ctx->color(rhsValTy->to_string()) +
                         " whose underlying type is " +
                         ctx->color(rhsValTy->as_choice()->get_underlying_type()->to_string()),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is an unsigned integer of type " + ctx->color(lhsValTy->to_string()) +
                         " but the operand on the right is of the choice type " + ctx->color(rhsValTy->to_string()) +
                         " whose underlying type is a signed integer type",
                     fileRange);
        }
      } else if (rhsValTy->is_unsigned_integer()) {
        ctx->Error("Unsigned integers in this binary operation have different "
                   "bitwidths. Cast the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValTy->is_integer()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a signed integer. Please check logic or cast one side to the "
                   "other type if this was intentional",
                   fileRange);
      } else if (rhsValTy->is_float()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a floating point number. Please check logic or cast one side to "
                   "the other type if this was intentional",
                   fileRange);
      } else {
        // FIXME - Support side flipped operator
        ctx->Error("No operator found that matches both operand types. The left hand "
                   "side is " +
                       ctx->color(lhsType->to_string()) + ", and the right hand side is " +
                       ctx->color(rhsType->to_string()),
                   fileRange);
      }
    }
  } else if (lhsValTy->is_bool() && rhsValTy->is_bool()) {
    auto            lhsConst = lhsEmit->get_llvm_constant();
    auto            rhsConst = rhsEmit->get_llvm_constant();
    llvm::Constant* llRes    = nullptr;
    switch (opr) {
      case Op::Or: {
        llRes = llvm::ConstantExpr::getOr(lhsConst, rhsConst);
        break;
      }
      case Op::And: {
        llRes = llvm::ConstantExpr::getAnd(lhsConst, rhsConst);
        break;
      }
      case Op::equalTo: {
        llRes = llvm::ConstantExpr::getICmp(llvm::ICmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
        break;
      }
      case Op::notEqualTo: {
        llRes = llvm::ConstantExpr::getICmp(llvm::ICmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
        break;
      }
      default: {
        ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
                       " is not supported for expressions of type " + ctx->color(lhsValTy->to_string()),
                   fileRange);
      }
    }
    return ir::PrerunValue::get(llRes, lhsValTy);
  } else if (lhsValTy->is_float()) {
    auto            lhsConst = lhsEmit->get_llvm_constant();
    auto            rhsConst = rhsEmit->get_llvm_constant();
    llvm::Constant* llRes    = nullptr;
    ir::Type*       resType  = lhsType;
    if (lhsType->is_same(rhsType)) {
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (opr) {
        case Op::add: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFAdd(lhsConst, rhsConst));
          break;
        }
        case Op::subtract: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFSub(lhsConst, rhsConst));
          break;
        }
        case Op::multiply: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFMul(lhsConst, rhsConst));
          break;
        }
        case Op::divide: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFDiv(lhsConst, rhsConst));
          // ctx->getMod()->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::remainder: {
          llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFRem(lhsConst, rhsConst));
          // ctx->getMod()->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
          break;
        }
        case Op::equalTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OEQ, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_ONE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OLT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OGT, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OLE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = llvm::ConstantExpr::getFCmp(llvm::CmpInst::Predicate::FCMP_OGE, lhsConst, rhsConst);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
                         " is not supported for expressions of type " + ctx->color(lhsType->to_string()) + " and " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      }
      return ir::PrerunValue::get(llRes, resType);
    } else {
      if (rhsType->is_float()) {
        ctx->Error(
            "Left hand side of the expression is " + lhsType->to_string() + " and right hand side is " +
                rhsType->to_string() +
                ". The floating point types do not match. Convert either value to the type of the other one if this was intentional",
            fileRange);
      } else if (rhsType->is_integer()) {
        ctx->Error("The right hand side of the expression is a signed integer. "
                   "If this was intentional, convert the integer value to " +
                       ctx->color(lhsType->to_string()),
                   fileRange);
      } else if (rhsType->is_unsigned_integer()) {
        ctx->Error("The right hand side of the expression is an unsigned integer. "
                   "If this was intentional, convert the unsigned integer value to " +
                       ctx->color(lhsType->to_string()),
                   fileRange);
      } else {
        ctx->Error("Left hand side of the expression is of type " + ctx->color(lhsType->to_string()) +
                       " and right hand side is of type " + rhsType->to_string() + ". Please check the logic.",
                   fileRange);
      }
    }
  } else if (lhsType->is_string_slice() && rhsType->is_string_slice()) {
    if (opr == Op::equalTo) {
      return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                                         lhsEmit->is_equal_to(ctx->irCtx, rhsEmit) ? 1u : 0u),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else if (opr == Op::notEqualTo) {
      return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                                         lhsEmit->is_equal_to(ctx->irCtx, rhsEmit) ? 0u : 1u),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else {
      ctx->Error("Unsupported operator " + ctx->color(operator_to_string(opr)) + " for operands of type " +
                     ctx->color(lhsType->to_string()),
                 fileRange);
    }
  } else if (lhsType->is_choice() && lhsType->is_same(rhsType)) {
    auto chTy     = lhsType->as_choice();
    auto lhsConst = lhsEmit->get_llvm_constant();
    auto rhsConst = rhsEmit->get_llvm_constant();
    if (opr == Op::equalTo) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else if (opr == Op::notEqualTo) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else if (opr == Op::lessThan) {
      if (chTy->has_negative_values()) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst),
                                    ir::UnsignedType::getBool(ctx->irCtx));
      } else {
        ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst),
                             ir::UnsignedType::getBool(ctx->irCtx));
      }
    } else if (opr == Op::lessThanOrEqualTo) {
      if (chTy->has_negative_values()) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst),
                                    ir::UnsignedType::getBool(ctx->irCtx));
      } else {
        ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst),
                             ir::UnsignedType::getBool(ctx->irCtx));
      }
    } else if (opr == Op::greaterThan) {
      if (chTy->has_negative_values()) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst),
                                    ir::UnsignedType::getBool(ctx->irCtx));
      } else {
        ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst),
                             ir::UnsignedType::getBool(ctx->irCtx));
      }
    } else if (opr == Op::greaterThanEqualTo) {
      if (chTy->has_negative_values()) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst),
                                    ir::UnsignedType::getBool(ctx->irCtx));
      } else {
        ir::PrerunValue::get(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst),
                             ir::UnsignedType::getBool(ctx->irCtx));
      }
    } else {
      // FIXME - ?? Separate type for bitwise operations
      if (opr == Op::bitwiseAnd) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getAnd(lhsConst, rhsConst), chTy->get_underlying_type());
      } else if (opr == Op::bitwiseOr) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getOr(lhsConst, rhsConst), chTy->get_underlying_type());
      } else if (opr == Op::bitwiseXor) {
        return ir::PrerunValue::get(llvm::ConstantExpr::getXor(lhsConst, rhsConst), chTy->get_underlying_type());
      } else {
        ctx->Error("Binary operator " + ctx->color(operator_to_string(opr)) +
                       " is not supported for expressions of type " + ctx->color(lhsType->to_string()),
                   fileRange);
      }
    }
  } else if (lhsType->is_choice() &&
             (lhsType->as_choice()->has_negative_values() ? rhsValTy->is_integer() : rhsValTy->is_unsigned_integer())) {
    if (opr == Op::bitwiseAnd) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getAnd(lhsEmit->get_llvm(), rhsEmit->get_llvm()),
                                  lhsType->as_choice()->get_underlying_type());
    } else if (opr == Op::bitwiseOr) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getOr(lhsEmit->get_llvm(), rhsEmit->get_llvm()),
                                  lhsType->as_choice()->get_underlying_type());
    } else if (opr == Op::bitwiseXor) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getXor(lhsEmit->get_llvm(), rhsEmit->get_llvm()),
                                  lhsType->as_choice()->get_underlying_type());
    } else {
      ctx->Error("Left hand side is of choice type " + ctx->color(lhsType->to_string()) +
                     " and right hand side is of type " + ctx->color(rhsType->to_string()) + ". Binary operator " +
                     ctx->color(operator_to_string(opr)) + " is not supported for these operands",
                 fileRange);
    }
  } else if (lhsValTy->is_pointer() && rhsValTy->is_pointer()) {
    if (lhsValTy->as_pointer()->get_subtype()->is_same(rhsValTy->as_pointer()->get_subtype()) &&
        (lhsValTy->as_pointer()->isMulti() == rhsValTy->as_pointer()->isMulti())) {
      llvm::Constant* finalCondition = nullptr;
      if (opr != Op::equalTo && opr != Op::notEqualTo) {
        ctx->Error("Unsupported operator " + ctx->color(operator_to_string(opr)) + " for expressions of type " +
                       ctx->color(lhsType->to_string()) + " and " + ctx->color(rhsType->to_string()),
                   fileRange);
      }
      finalCondition = llvm::ConstantExpr::getICmp(
          opr == Op::equalTo ? llvm::CmpInst::Predicate::ICMP_EQ : llvm::CmpInst::Predicate::ICMP_NE,
          llvm::ConstantExpr::getSub(llvm::ConstantExpr::getPtrToInt(lhsValTy->as_pointer()->isMulti()
                                                                         ? lhsEmit->get_llvm()->getAggregateElement(0u)
                                                                         : lhsEmit->get_llvm(),
                                                                     llvm::Type::getInt64Ty(ctx->irCtx->llctx)),
                                     llvm::ConstantExpr::getPtrToInt(lhsValTy->as_pointer()->isMulti()
                                                                         ? rhsEmit->get_llvm()->getAggregateElement(0u)
                                                                         : rhsEmit->get_llvm(),
                                                                     llvm::Type::getInt64Ty(ctx->irCtx->llctx))),
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u));
      if (lhsValTy->as_pointer()->isMulti()) {
        finalCondition = llvm::ConstantExpr::getAnd(
            llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, lhsEmit->get_llvm()->getAggregateElement(1u),
                                        rhsEmit->get_llvm()->getAggregateElement(1u)),
            finalCondition);
      }
      return ir::PrerunValue::get(llvm::ConstantFoldConstant(finalCondition, ctx->irCtx->dataLayout.value()),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else {
      ctx->Error("The operands have different pointer types. The left hand side is of type " +
                     ctx->color(lhsType->to_string()) + " and the right hand side is of type " +
                     ctx->color(rhsType->to_string()),
                 fileRange);
    }
  } else {
    ctx->Error("Unsupported operand types for operator " + ctx->color(operator_to_string(opr)) +
                   ". The left hand side is of type " + ctx->color(lhsType->to_string()) +
                   " and the right hand side is of type " + ctx->color(rhsType->to_string()),
               fileRange);
  }
}
String PrerunBinaryOp::to_string() const {
  return lhs->to_string() + " " + operator_to_string(opr) + " " + rhs->to_string();
}

Json PrerunBinaryOp::to_json() const {
  return Json()
      ._("nodeType", "prerunBinaryOp")
      ._("lhs", lhs->to_json())
      ._("operator", operator_to_string(opr))
      ._("rhs", rhs->to_json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast