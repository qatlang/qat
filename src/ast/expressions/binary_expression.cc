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

ir::Value* BinaryExpression::emit(EmitCtx* ctx) {
  SHOW("Lhs node type is " << (int)lhs->nodeType() << " and rhs node type is " << (int)rhs->nodeType())
  ir::Value* lhsEmit = nullptr;
  ir::Value* rhsEmit = nullptr;
  if (lhs->nodeType() == NodeType::DEFAULT) {
    rhsEmit = rhs->emit(ctx);
    lhs->as_type_inferrable()->set_inference_type(
        rhsEmit->is_reference() ? rhsEmit->get_ir_type()->as_reference()->get_subtype() : rhsEmit->get_ir_type());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::DEFAULT) {
    lhsEmit = lhs->emit(ctx);
    rhs->as_type_inferrable()->set_inference_type(
        lhsEmit->is_reference() ? lhsEmit->get_ir_type()->as_reference()->get_subtype() : lhsEmit->get_ir_type());
    rhsEmit = rhs->emit(ctx);
  } else if (lhs->nodeType() == NodeType::NULL_POINTER) {
    rhsEmit = rhs->emit(ctx);
    lhs->as_type_inferrable()->set_inference_type(rhsEmit->get_ir_type());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->nodeType() == NodeType::NULL_POINTER) {
    lhsEmit = lhs->emit(ctx);
    rhs->as_type_inferrable()->set_inference_type(lhsEmit->get_ir_type());
    rhsEmit = rhs->emit(ctx);
  } else if ((lhs->nodeType() == NodeType::INTEGER_LITERAL || lhs->nodeType() == NodeType::UNSIGNED_LITERAL ||
              lhs->nodeType() == NodeType::FLOAT_LITERAL || lhs->nodeType() == NodeType::CUSTOM_FLOAT_LITERAL ||
              lhs->nodeType() == NodeType::CUSTOM_INTEGER_LITERAL) &&
             expect_same_operand_types(op)) {
    rhsEmit = rhs->emit(ctx);
    lhs->as_type_inferrable()->set_inference_type(rhsEmit->get_ir_type());
    lhsEmit = lhs->emit(ctx);
  } else if (rhs->has_type_inferrance() && expect_same_operand_types(op)) {
    lhsEmit    = lhs->emit(ctx);
    auto lhsTy = lhsEmit->get_ir_type()->is_reference() ? lhsEmit->get_ir_type()->as_reference()->get_subtype()
                                                        : lhsEmit->get_ir_type();
    if (lhsTy->is_ctype()) {
      lhsTy = lhsTy->as_ctype()->get_subtype();
    }
    if (lhsTy->is_integer() || lhsTy->is_unsigned_integer() || lhsTy->is_float()) {
      rhs->as_type_inferrable()->set_inference_type(lhsEmit->get_ir_type());
    }
    rhsEmit = rhs->emit(ctx);
  } else {
    lhsEmit = lhs->emit(ctx);
    rhsEmit = rhs->emit(ctx);
  }

  SHOW("Operator is: " << operator_to_string(op))

  ir::Type*    lhsType          = lhsEmit->get_ir_type();
  ir::Type*    rhsType          = rhsEmit->get_ir_type();
  llvm::Value* lhsVal           = lhsEmit->get_llvm();
  llvm::Value* rhsVal           = rhsEmit->get_llvm();
  auto         referenceHandler = [&]() {
    SHOW("Reference handler ::")
    SHOW("Loading implicit LHS")
    lhsEmit->load_ghost_reference(ctx->irCtx->builder);
    SHOW("Loaded LHS")
    lhsVal = lhsEmit->get_llvm();
    SHOW("Loading implicit RHS")
    rhsEmit->load_ghost_reference(ctx->irCtx->builder);
    SHOW("Loaded RHS")
    rhsVal = rhsEmit->get_llvm();
    if (lhsEmit->is_reference()) {
      SHOW("LHS is reference")
      lhsType = lhsType->as_reference()->get_subtype();
      SHOW("LHS type is: " << lhsType->to_string())
      lhsVal = ctx->irCtx->builder.CreateLoad(lhsType->get_llvm_type(), lhsVal, false);
    }
    if (rhsEmit->is_reference()) {
      SHOW("RHS is reference")
      rhsType = rhsType->as_reference()->get_subtype();
      SHOW("RHS type is: " << rhsType->to_string())
      rhsVal = ctx->irCtx->builder.CreateLoad(rhsType->get_llvm_type(), rhsVal, false);
    }
  };
  auto lhsValueType = lhsType->is_reference() ? lhsType->as_reference()->get_subtype() : lhsType;
  if (lhsValueType->is_ctype()) {
    lhsValueType = lhsValueType->as_ctype()->get_subtype();
  }
  auto rhsValueType = rhsType->is_reference() ? rhsType->as_reference()->get_subtype() : rhsType;
  if (rhsValueType->is_ctype()) {
    rhsValueType = rhsValueType->as_ctype()->get_subtype();
  }
  if (lhsValueType->is_bool() && rhsValueType->is_bool()) {
    referenceHandler();
    if (op == Op::equalTo) {
      return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false)
          ->with_range(fileRange);
    } else if (op == Op::notEqualTo) {
      return ir::Value::get(ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false)
          ->with_range(fileRange);
    } else if (op == Op::And) {
      return ir::Value::get(ctx->irCtx->builder.CreateLogicalAnd(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false)
          ->with_range(fileRange);
    } else if (op == Op::Or) {
      return ir::Value::get(ctx->irCtx->builder.CreateLogicalOr(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false);
    } else {
      ctx->Error("Unsupported operator " + ctx->color(operator_to_string(op)) + " for expressions of type " +
                     ctx->color(lhsType->to_string()),
                 fileRange);
    }
  } else if (lhsValueType->is_integer()) {
    referenceHandler();
    SHOW("Integer binary operation: " << operator_to_string(op))
    if (lhsType->is_same(rhsType) ||
        (!lhsType->is_ctype() && !rhsType->is_ctype() && lhsValueType->is_same(rhsValueType))) {
      SHOW("Operand types match")
      llvm::Value* llRes   = nullptr;
      ir::Type*    resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (op) {
        case Op::add: {
          llRes = ctx->irCtx->builder.CreateAdd(lhsVal, rhsVal);
          break;
        }
        case Op::subtract: {
          llRes = ctx->irCtx->builder.CreateSub(lhsVal, rhsVal);
          break;
        }
        case Op::multiply: {
          llRes = ctx->irCtx->builder.CreateMul(lhsVal, rhsVal);
          break;
        }
        case Op::divide: {
          llRes = ctx->irCtx->builder.CreateSDiv(lhsVal, rhsVal);
          break;
        }
        case Op::remainder: {
          llRes = ctx->irCtx->builder.CreateSRem(lhsVal, rhsVal);
          break;
        }
        case Op::equalTo: {
          llRes   = ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->irCtx->builder.CreateICmpSLT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->irCtx->builder.CreateICmpSGT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpSLE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpSGE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseOr: {
          llRes = ctx->irCtx->builder.CreateOr(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseXor: {
          llRes = ctx->irCtx->builder.CreateXor(lhsVal, rhsVal);
          break;
        }
        case Op::logicalLeftShift: {
          llRes = ctx->irCtx->builder.CreateShl(lhsVal, rhsVal);
          break;
        }
        case Op::logicalRightShift: {
          llRes = ctx->irCtx->builder.CreateLShr(lhsVal, rhsVal);
          break;
        }
        case Op::arithmeticRightShift: {
          llRes = ctx->irCtx->builder.CreateAShr(lhsVal, rhsVal);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->color(operator_to_string(op)) +
                         " is not supported for expressions of type " +
                         ctx->color(lhsEmit->get_ir_type()->to_string()) + " and " +
                         ctx->color(rhsEmit->get_ir_type()->to_string()),
                     fileRange);
        }
      }
      SHOW("Returning IR Value llres " << llRes << " resType " << resType)
      return ir::Value::get(llRes, resType, false)->with_range(fileRange);
    } else {
      if (rhsValueType->is_choice() && (rhsValueType->as_choice()->get_underlying_type()->is_same(lhsValueType))) {
        if (op == Op::bitwiseAnd) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else if (op == Op::bitwiseOr) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateOr(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else if (op == Op::bitwiseXor) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateXor(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else {
          ctx->Error("Unsupported operator " + ctx->color(operator_to_string(op)) + " for left hand side type " +
                         ctx->color(lhsValueType->to_string()) + " and right hand side type " +
                         ctx->color(rhsValueType->to_string()),
                     fileRange);
        }
      } else if (rhsValueType->is_choice()) {
        if (rhsValueType->as_choice()->has_negative_values()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->color(std::to_string(lhsValueType->as_integer()->get_bitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->color(rhsValueType->to_string()) + " whose underlying type is " +
                         ctx->color(rhsValueType->as_choice()->get_underlying_type()->to_string()),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is a signed integer of type " + ctx->color(lhsValueType->to_string()) +
                         " but the operand on the right is of the choice type " +
                         ctx->color(rhsValueType->to_string()) + " whose underlying type is an unsigned integer type",
                     fileRange);
        }
      } else if (rhsValueType->is_integer()) {
        ctx->Error("Signed integers in this binary operation have different bitwidths."
                   " It is recommended to convert the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValueType->is_unsigned_integer()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "an unsigned integer. Please check logic or convert one side "
                   "to the other type if this was intentional",
                   fileRange);
      } else if (rhsValueType->is_float()) {
        ctx->Error("Left hand side is a signed integer and right hand side is "
                   "a floating point number. Please check logic or convert one "
                   "side to the other type if this was intentional",
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
  } else if (lhsValueType->is_unsigned_integer()) {
    SHOW("Unsigned integer binary operation")
    referenceHandler();
    if (lhsType->is_same(rhsType) ||
        (!lhsType->is_ctype() && !rhsType->is_ctype() && lhsValueType->is_same(rhsValueType))) {
      llvm::Value* llRes   = nullptr;
      ir::Type*    resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (op) {
        case Op::add: {
          llRes = ctx->irCtx->builder.CreateAdd(lhsVal, rhsVal);
          break;
        }
        case Op::subtract: {
          llRes = ctx->irCtx->builder.CreateSub(lhsVal, rhsVal);
          break;
        }
        case Op::multiply: {
          llRes = ctx->irCtx->builder.CreateMul(lhsVal, rhsVal);
          break;
        }
        case Op::divide: {
          llRes = ctx->irCtx->builder.CreateUDiv(lhsVal, rhsVal);
          break;
        }
        case Op::remainder: {
          llRes = ctx->irCtx->builder.CreateURem(lhsVal, rhsVal);
          break;
        }
        case Op::equalTo: {
          llRes   = ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->irCtx->builder.CreateICmpULT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->irCtx->builder.CreateICmpUGT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpULE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->irCtx->builder.CreateICmpUGE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::bitwiseAnd: {
          llRes = ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseOr: {
          llRes = ctx->irCtx->builder.CreateOr(lhsVal, rhsVal);
          break;
        }
        case Op::bitwiseXor: {
          llRes = ctx->irCtx->builder.CreateXor(lhsVal, rhsVal);
          break;
        }
        case Op::logicalLeftShift: {
          llRes = ctx->irCtx->builder.CreateShl(lhsVal, rhsVal);
          break;
        }
        case Op::logicalRightShift: {
          llRes = ctx->irCtx->builder.CreateLShr(lhsVal, rhsVal);
          break;
        }
        case Op::arithmeticRightShift: {
          llRes = ctx->irCtx->builder.CreateAShr(lhsVal, rhsVal);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->color(operator_to_string(op)) +
                         " is not supported for expressions of type " + ctx->color(lhsType->to_string()) + " and " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      return ir::Value::get(llRes, resType, false)->with_range(fileRange);
    } else {
      if (rhsValueType->is_choice() && !rhsValueType->as_choice()->has_negative_values() &&
          (rhsValueType->as_choice()->get_underlying_type()->is_same(lhsValueType))) {
        if (op == Op::bitwiseAnd) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else if (op == Op::bitwiseOr) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateOr(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else if (op == Op::bitwiseXor) {
          referenceHandler();
          return ir::Value::get(ctx->irCtx->builder.CreateXor(lhsVal, rhsVal), lhsValueType, false)
              ->with_range(fileRange);
        } else {
          ctx->Error("Unsupported operator " + ctx->color(operator_to_string(op)) + " for left hand side type " +
                         ctx->color(lhsType->to_string()) + " and right hand side type " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      } else if (rhsValueType->is_choice()) {
        if (!rhsValueType->as_choice()->has_negative_values()) {
          ctx->Error("The bitwidth of the operand on the left is " +
                         ctx->color(std::to_string(lhsValueType->as_unsigned_integer()->getBitwidth())) +
                         ", but the operand on the right is of the choice type " +
                         ctx->color(rhsValueType->to_string()) + " whose underlying type is " +
                         ctx->color(rhsValueType->as_choice()->get_underlying_type()->to_string()),
                     fileRange);
        } else {
          ctx->Error("The operand on the left is an unsigned integer of type " + ctx->color(lhsValueType->to_string()) +
                         " but the operand on the right is of the choice type " +
                         ctx->color(rhsValueType->to_string()) + " whose underlying type is a signed integer type",
                     fileRange);
        }
      } else if (rhsValueType->is_unsigned_integer()) {
        ctx->Error("Unsigned integers in this binary operation have different "
                   "bitwidths. Cast the operand with smaller bitwidth to the bigger "
                   "bitwidth to prevent potential loss of data and logical errors",
                   fileRange);
      } else if (rhsValueType->is_integer()) {
        ctx->Error("Left hand side is an unsigned integer and right hand side is "
                   "a signed integer. Please check logic or cast one side to the "
                   "other type if this was intentional",
                   fileRange);
      } else if (rhsValueType->is_float()) {
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
  } else if (lhsValueType->is_float()) {
    SHOW("Float binary operation")
    referenceHandler();
    if (lhsType->is_same(rhsType) ||
        (!lhsType->is_ctype() && !rhsType->is_ctype() && lhsValueType->is_same(rhsValueType))) {
      llvm::Value* llRes   = nullptr;
      ir::Type*    resType = lhsType;
      // NOLINTNEXTLINE(clang-diagnostic-switch)
      switch (op) {
        case Op::add: {
          llRes = ctx->irCtx->builder.CreateFAdd(lhsVal, rhsVal);
          break;
        }
        case Op::subtract: {
          llRes = ctx->irCtx->builder.CreateFSub(lhsVal, rhsVal);
          break;
        }
        case Op::multiply: {
          llRes = ctx->irCtx->builder.CreateFMul(lhsVal, rhsVal);
          break;
        }
        case Op::divide: {
          llRes = ctx->irCtx->builder.CreateFDiv(lhsVal, rhsVal);
          if (llvm::Triple(cli::Config::get()->get_target_triple()).isOSLinux()) {
            ctx->mod->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
          }
          break;
        }
        case Op::remainder: {
          llRes = ctx->irCtx->builder.CreateFRem(lhsVal, rhsVal);
          if (llvm::Triple(cli::Config::get()->get_target_triple()).isOSLinux()) {
            ctx->mod->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
          }
          break;
        }
        case Op::equalTo: {
          llRes   = ctx->irCtx->builder.CreateFCmpOEQ(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::notEqualTo: {
          llRes   = ctx->irCtx->builder.CreateFCmpONE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThan: {
          llRes   = ctx->irCtx->builder.CreateFCmpOLT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThan: {
          llRes   = ctx->irCtx->builder.CreateFCmpOGT(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::lessThanOrEqualTo: {
          llRes   = ctx->irCtx->builder.CreateFCmpOLE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        case Op::greaterThanEqualTo: {
          llRes   = ctx->irCtx->builder.CreateFCmpOGE(lhsVal, rhsVal);
          resType = ir::UnsignedType::getBool(ctx->irCtx);
          break;
        }
        default: {
          ctx->Error("The operator " + ctx->color(operator_to_string(op)) +
                         " is not supported for expressions of type " + ctx->color(lhsType->to_string()) + " and " +
                         ctx->color(rhsType->to_string()),
                     fileRange);
        }
      }
      return ir::Value::get(llRes, resType, false)->with_range(fileRange);
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
  } else if (lhsValueType->is_string_slice() && rhsValueType->is_string_slice()) {
    if (op == Op::equalTo || op == Op::notEqualTo) {
      // NOLINTBEGIN(readability-isolate-declaration)
      llvm::Value *lhsBuff, *lhsCount, *rhsBuff, *rhsCount;
      bool         isConstantLHS = false, isConstantRHS = false;
      // NOLINTEND(readability-isolate-declaration)
      auto* Ty64Int = llvm::Type::getInt64Ty(ctx->irCtx->llctx);
      if (lhsEmit->is_llvm_constant()) {
        lhsBuff       = lhsEmit->get_llvm_constant()->getAggregateElement(0u);
        lhsCount      = lhsEmit->get_llvm_constant()->getAggregateElement(1u);
        isConstantLHS = true;
      } else {
        if (lhsType->is_reference()) {
          lhsEmit->load_ghost_reference(ctx->irCtx->builder);
        } else if (!lhsEmit->is_ghost_reference()) {
          lhsEmit = lhsEmit->make_local(ctx, None, lhs->fileRange);
        }
        lhsBuff = ctx->irCtx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(ctx->irCtx->llctx),
            ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
                                                lhsEmit->get_llvm(), 0u));
        lhsCount = ctx->irCtx->builder.CreateLoad(
            Ty64Int, ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
                                                         lhsEmit->get_llvm(), 1u));
      }
      if (rhsEmit->is_llvm_constant()) {
        rhsBuff       = rhsEmit->get_llvm_constant()->getAggregateElement(0u);
        rhsCount      = rhsEmit->get_llvm_constant()->getAggregateElement(1u);
        isConstantRHS = true;
      } else {
        if (rhsType->is_reference()) {
          rhsEmit->load_ghost_reference(ctx->irCtx->builder);
        } else if (!rhsEmit->is_ghost_reference()) {
          rhsEmit = rhsEmit->make_local(ctx, None, rhs->fileRange);
        }
        rhsBuff = ctx->irCtx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(ctx->irCtx->llctx),
            ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
                                                rhsEmit->get_llvm(), 0u));
        rhsCount = ctx->irCtx->builder.CreateLoad(
            Ty64Int, ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
                                                         rhsEmit->get_llvm(), 1u));
      }
      if (isConstantLHS && isConstantRHS) {
        SHOW("Both string slices are constant")
        auto strCmpRes = ir::Logic::compareConstantStrings(
            llvm::cast<llvm::Constant>(lhsBuff), llvm::cast<llvm::Constant>(lhsCount),
            llvm::cast<llvm::Constant>(rhsBuff), llvm::cast<llvm::Constant>(rhsCount), ctx->irCtx->llctx);
        return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                                           (op == Op::equalTo) ? strCmpRes : !strCmpRes),
                                    ir::UnsignedType::getBool(ctx->irCtx))
            ->with_range(fileRange);
      }
      auto* curr              = ctx->get_fn()->get_block();
      auto* lenCheckTrueBlock = new ir::Block(ctx->get_fn(), curr);
      auto* strCmpTrueBlock   = new ir::Block(ctx->get_fn(), curr);
      auto* strCmpFalseBlock  = new ir::Block(ctx->get_fn(), curr);
      auto* iterCondBlock     = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
      auto* iterTrueBlock     = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
      auto* iterIncrBlock     = new ir::Block(ctx->get_fn(), iterTrueBlock);
      auto* iterFalseBlock    = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
      auto* restBlock         = new ir::Block(ctx->get_fn(), curr->get_parent());
      restBlock->link_previous_block(curr);
      auto* Ty8Int         = llvm::Type::getInt8Ty(ctx->irCtx->llctx);
      auto* qatStrCmpIndex = ctx->get_fn()->get_str_comparison_index();
      // NOTE - Length equality check
      ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpEQ(lhsCount, rhsCount),
                                       lenCheckTrueBlock->get_bb(), strCmpFalseBlock->get_bb());
      //
      // NOTE - Length matches
      lenCheckTrueBlock->set_active(ctx->irCtx->builder);
      ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u),
                                      qatStrCmpIndex->get_llvm());
      (void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
      //
      // NOTE - Checking the iteration count
      iterCondBlock->set_active(ctx->irCtx->builder);
      ctx->irCtx->builder.CreateCondBr(
          ctx->irCtx->builder.CreateICmpULT(ctx->irCtx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->get_llvm()),
                                            lhsCount),
          iterTrueBlock->get_bb(), iterFalseBlock->get_bb());
      //
      // NOTE - Iteration check is true
      iterTrueBlock->set_active(ctx->irCtx->builder);
      ctx->irCtx->builder.CreateCondBr(
          ctx->irCtx->builder.CreateICmpEQ(
              ctx->irCtx->builder.CreateLoad(
                  Ty8Int, ctx->irCtx->builder.CreateInBoundsGEP(
                              Ty8Int, lhsBuff, {ctx->irCtx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->get_llvm())})),
              ctx->irCtx->builder.CreateLoad(
                  Ty8Int, ctx->irCtx->builder.CreateInBoundsGEP(
                              Ty8Int, rhsBuff, {ctx->irCtx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->get_llvm())}))),
          iterIncrBlock->get_bb(), iterFalseBlock->get_bb());
      //
      // NOTE - Increment string slice iteration count
      iterIncrBlock->set_active(ctx->irCtx->builder);
      ctx->irCtx->builder.CreateStore(
          ctx->irCtx->builder.CreateAdd(ctx->irCtx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->get_llvm()),
                                        llvm::ConstantInt::get(Ty64Int, 1u)),
          qatStrCmpIndex->get_llvm());
      (void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
      //
      // NOTE - Iteration check is false, time to check if the count matches
      iterFalseBlock->set_active(ctx->irCtx->builder);
      ctx->irCtx->builder.CreateCondBr(
          ctx->irCtx->builder.CreateICmpEQ(lhsCount,
                                           ctx->irCtx->builder.CreateLoad(Ty64Int, qatStrCmpIndex->get_llvm())),
          strCmpTrueBlock->get_bb(), strCmpFalseBlock->get_bb());
      //
      // NOTE - Merging branches to the resume block
      strCmpTrueBlock->set_active(ctx->irCtx->builder);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      strCmpFalseBlock->set_active(ctx->irCtx->builder);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      //
      // NOTE - Control flow resumes
      restBlock->set_active(ctx->irCtx->builder);
      auto* Ty1Int    = llvm::Type::getInt1Ty(ctx->irCtx->llctx);
      auto* strCmpRes = ctx->irCtx->builder.CreatePHI(Ty1Int, 2);
      strCmpRes->addIncoming(llvm::ConstantInt::get(Ty1Int, (op == Op::equalTo) ? 1u : 0u), strCmpTrueBlock->get_bb());
      strCmpRes->addIncoming(llvm::ConstantInt::get(Ty1Int, (op == Op::equalTo) ? 0u : 1u), strCmpFalseBlock->get_bb());
      return ir::Value::get(strCmpRes, ir::UnsignedType::getBool(ctx->irCtx), false)->with_range(fileRange);
    } else {
      ctx->Error("String slices do not support the " + ctx->color(operator_to_string(op)) + " operator", fileRange);
    }
  } else if (lhsValueType->is_mark() && rhsValueType->is_mark()) {
    SHOW("LHS type is: " << lhsType->to_string() << " and RHS type is: " << rhsType->to_string())
    if (lhsValueType->as_mark()->get_subtype()->is_same(rhsValueType->as_mark()->get_subtype()) &&
        (lhsValueType->as_mark()->isSlice() == rhsValueType->as_mark()->isSlice())) {
      if (lhsValueType->as_mark()->isSlice()) {
        bool isLHSRef = false;
        SHOW("LHS side")
        if (lhsEmit->is_reference()) {
          lhsEmit->load_ghost_reference(ctx->irCtx->builder);
          isLHSRef = true;
        } else if (lhsEmit->is_ghost_reference()) {
          isLHSRef = true;
        }
        auto* ptrType = lhsValueType->as_mark();
        lhsType       = ptrType;
        auto resPtrTy = ir::MarkType::get(false, ptrType->get_subtype(), ptrType->isNonNullable(),
                                          ir::MarkOwner::OfAnonymous(), false, ctx->irCtx);
        if (lhsEmit->is_prerun_value()) {
          lhsEmit = ir::PrerunValue::get(lhsEmit->get_llvm_constant()->getAggregateElement(0u), resPtrTy);
        } else if (isLHSRef) {
          lhsEmit =
              ir::Value::get(ctx->irCtx->builder.CreateLoad(resPtrTy->get_llvm_type(),
                                                            ctx->irCtx->builder.CreateStructGEP(
                                                                ptrType->get_llvm_type(), lhsEmit->get_llvm(), 0u)),
                             resPtrTy, false);
        } else {
          lhsEmit = ir::Value::get(ctx->irCtx->builder.CreateExtractValue(lhsEmit->get_llvm(), {0u}), resPtrTy, false);
        }
        lhsVal = lhsEmit->get_llvm();
        SHOW("Set LhsEmit")
      } else if (lhsEmit->is_reference() || lhsEmit->is_ghost_reference()) {
        lhsEmit->load_ghost_reference(ctx->irCtx->builder);
        if (lhsEmit->is_reference()) {
          lhsEmit = ir::Value::get(ctx->irCtx->builder.CreateLoad(lhsValueType->get_llvm_type(), lhsEmit->get_llvm()),
                                   lhsValueType, false);
        }
        lhsType = lhsValueType;
        lhsVal  = lhsEmit->get_llvm();
      }
      if (rhsValueType->as_mark()->isSlice()) {
        bool isRHSRef = false;
        SHOW("RHS side")
        if (rhsEmit->is_reference()) {
          SHOW("Loading RHS")
          rhsEmit->load_ghost_reference(ctx->irCtx->builder);
          isRHSRef = true;
        } else if (rhsEmit->is_ghost_reference()) {
          isRHSRef = true;
        }
        auto* ptrType = rhsValueType->as_mark();
        rhsType       = ptrType;
        auto resPtrTy = ir::MarkType::get(false, ptrType->get_subtype(), ptrType->isNonNullable(),
                                          ir::MarkOwner::OfAnonymous(), false, ctx->irCtx);
        if (rhsEmit->is_prerun_value()) {
          rhsEmit = ir::PrerunValue::get(rhsEmit->get_llvm_constant()->getAggregateElement(0u), resPtrTy);
        } else if (isRHSRef) {
          rhsEmit =
              ir::Value::get(ctx->irCtx->builder.CreateLoad(resPtrTy->get_llvm_type(),
                                                            ctx->irCtx->builder.CreateStructGEP(
                                                                ptrType->get_llvm_type(), rhsEmit->get_llvm(), 0u)),
                             resPtrTy, false);
        } else {
          rhsEmit = ir::Value::get(ctx->irCtx->builder.CreateExtractValue(rhsEmit->get_llvm(), {0u}), resPtrTy, false);
        }
        rhsVal = rhsEmit->get_llvm();
        SHOW("Set RhsEmit")
      } else if (rhsEmit->is_reference() || rhsEmit->is_ghost_reference()) {
        rhsEmit->load_ghost_reference(ctx->irCtx->builder);
        if (rhsEmit->is_reference()) {
          rhsEmit = ir::Value::get(ctx->irCtx->builder.CreateLoad(rhsValueType->get_llvm_type(), rhsEmit->get_llvm()),
                                   rhsValueType, false);
        }
        rhsType = rhsValueType;
        rhsVal  = rhsEmit->get_llvm();
      }
      SHOW("LHS type is: " << lhsType->to_string() << " and RHS type is: " << rhsType->to_string())
      auto ptrTy = lhsValueType->as_mark();
      if (op == Op::equalTo) {
        SHOW("Pointer is normal")
        return ir::Value::get(
                   ctx->irCtx->builder.CreateICmpEQ(
                       ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx), lhsVal, rhsVal),
                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)),
                   ir::UnsignedType::getBool(ctx->irCtx), false)
            ->with_range(fileRange);
      } else if (op == Op::notEqualTo) {
        SHOW("Pointer is normal")
        return ir::Value::get(
                   ctx->irCtx->builder.CreateICmpNE(
                       ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx), lhsVal, rhsVal),
                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)),
                   ir::UnsignedType::getBool(ctx->irCtx), false)
            ->with_range(fileRange);
      } else {
        ctx->Error("The operands are pointers, and the operation " + ctx->color(operator_to_string(op)) +
                       " is not supported for pointers",
                   fileRange);
      }
    } else {
      ctx->Error("The operands have different pointer types. The left hand side "
                 "is of type " +
                     ctx->color(lhsValueType->to_string()) + " and the right hand side is of type " +
                     ctx->color(rhsValueType->to_string()),
                 fileRange);
    }
  } else if (lhsValueType->is_choice() && rhsValueType->is_same(lhsValueType)) {
    referenceHandler();
    auto chTy = lhsValueType->as_choice();
    if (op == Op::equalTo) {
      return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false)
          ->with_range(fileRange);
    } else if (op == Op::notEqualTo) {
      return ir::Value::get(ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                            false)
          ->with_range(fileRange);
    } else if (op == Op::lessThan) {
      if (chTy->has_negative_values()) {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpSLT(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpULT(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      }
    } else if (op == Op::lessThanOrEqualTo) {
      if (chTy->has_negative_values()) {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpSLE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpULE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      }
    } else if (op == Op::greaterThan) {
      if (chTy->has_negative_values()) {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpSGT(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpUGT(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      }
    } else if (op == Op::greaterThanEqualTo) {
      if (chTy->has_negative_values()) {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpSGE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpUGE(lhsVal, rhsVal), ir::UnsignedType::getBool(ctx->irCtx),
                              false)
            ->with_range(fileRange);
      }
    } else {
      // FIXME - ?? Separate type for bitwise operations
      if (op == Op::bitwiseAnd) {
        return ir::Value::get(ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal), chTy->get_underlying_type(), false)
            ->with_range(fileRange);
      } else if (op == Op::bitwiseOr) {
        return ir::Value::get(ctx->irCtx->builder.CreateOr(lhsVal, rhsVal), chTy->get_underlying_type(), false)
            ->with_range(fileRange);
      } else if (op == Op::bitwiseXor) {
        return ir::Value::get(ctx->irCtx->builder.CreateXor(lhsVal, rhsVal), chTy->get_underlying_type(), false)
            ->with_range(fileRange);
      } else {
        ctx->Error("This binary operator is not supported for expressions of type " + ctx->color(lhsType->to_string()),
                   fileRange);
      }
    }
  } else if (lhsValueType->is_choice() && (lhsValueType->as_choice()->get_underlying_type()->is_same(rhsValueType))) {
    if (op == Op::bitwiseAnd) {
      referenceHandler();
      return ir::Value::get(ctx->irCtx->builder.CreateAnd(lhsVal, rhsVal),
                            lhsValueType->as_choice()->get_underlying_type(), false)
          ->with_range(fileRange);
    } else if (op == Op::bitwiseOr) {
      referenceHandler();
      return ir::Value::get(ctx->irCtx->builder.CreateOr(lhsVal, rhsVal),
                            lhsValueType->as_choice()->get_underlying_type(), false)
          ->with_range(fileRange);
    } else if (op == Op::bitwiseXor) {
      referenceHandler();
      return ir::Value::get(ctx->irCtx->builder.CreateXor(lhsVal, rhsVal),
                            lhsValueType->as_choice()->get_underlying_type(), false)
          ->with_range(fileRange);
    } else {
      ctx->Error("Left hand side is of choice type " + ctx->color(lhsValueType->to_string()) +
                     " and right hand side is of type " + ctx->color(rhsValueType->to_string()) + ". Binary operator " +
                     ctx->color(operator_to_string(op)) + " is not supported for these operands",
                 fileRange);
    }
  } else {
    if (lhsType->is_expanded() || (lhsType->is_reference() && lhsType->as_reference()->get_subtype()->is_expanded())) {
      SHOW("Expanded type binary operation")
      auto* eTy =
          lhsType->is_reference() ? lhsType->as_reference()->get_subtype()->as_expanded() : lhsType->as_expanded();
      auto OpStr    = operator_to_string(op);
      bool isVarExp = lhsType->is_reference() ? lhsType->as_reference()->isSubtypeVariable()
                                              : (lhsEmit->is_ghost_reference() ? lhsEmit->is_variable() : true);
      if ((isVarExp &&
           eTy->has_variation_binary_operator(
               OpStr, {rhsEmit->is_ghost_reference() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) ||
          eTy->has_normal_binary_operator(
              OpStr, {rhsEmit->is_ghost_reference() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) {
        SHOW("RHS is matched exactly")
        auto localID = lhsEmit->get_local_id();
        if (!lhsType->is_reference() && !lhsEmit->is_ghost_reference()) {
          lhsEmit = lhsEmit->make_local(ctx, None, lhs->fileRange);
        } else if (lhsType->is_reference()) {
          lhsEmit->load_ghost_reference(ctx->irCtx->builder);
        }
        auto* opFn =
            (isVarExp &&
             eTy->has_variation_binary_operator(
                 OpStr, {rhsEmit->is_ghost_reference() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType}))
                ? eTy->get_variation_binary_operator(
                      OpStr, {lhsEmit->is_ghost_reference() ? Maybe<bool>(lhsEmit->is_variable()) : None, rhsType})
                : eTy->get_normal_binary_operator(
                      OpStr, {lhsEmit->is_ghost_reference() ? Maybe<bool>(lhsEmit->is_variable()) : None, rhsType});
        if (!opFn->is_accessible(ctx->get_access_info())) {
          ctx->Error(String(isVarExp ? "Variation b" : "B") + "inary operator " + ctx->color(operator_to_string(op)) +
                         " of type " + ctx->color(eTy->get_full_name()) + " with right hand side type " +
                         ctx->color(rhsType->to_string()) + " is not accessible here",
                     fileRange);
        }
        rhsEmit->load_ghost_reference(ctx->irCtx->builder);
        return opFn->call(ctx->irCtx, {lhsEmit->get_llvm(), rhsEmit->get_llvm()}, localID, ctx->mod)
            ->with_range(fileRange);
      } else if (rhsType->is_reference() &&
                 ((isVarExp &&
                   eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()})) ||
                  eTy->has_normal_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()}))) {
        auto localID = rhsEmit->get_local_id();
        rhsEmit->load_ghost_reference(ctx->irCtx->builder);
        SHOW("RHS is matched as subtype of reference")
        if (!lhsType->is_reference() && !lhsEmit->is_ghost_reference()) {
          lhsEmit = lhsEmit->make_local(ctx, None, lhs->fileRange);
        }
        auto* opFn =
            (isVarExp && eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()}))
                ? eTy->get_variation_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()})
                : eTy->get_normal_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()});
        if (!opFn->is_accessible(ctx->get_access_info())) {
          ctx->Error("Operator " + ctx->color(operator_to_string(op)) + " of type " + ctx->color(eTy->get_full_name()) +
                         " with right hand side type: " + ctx->color(rhsType->to_string()) + " is not accessible here",
                     fileRange);
        }
        return opFn
            ->call(ctx->irCtx,
                   {lhsEmit->get_llvm(),
                    ctx->irCtx->builder.CreateLoad(rhsType->as_reference()->get_subtype()->get_llvm_type(),
                                                   rhsEmit->get_llvm())},
                   localID, ctx->mod)
            ->with_range(fileRange);
      } else {
        if (!isVarExp &&
            eTy->has_variation_binary_operator(
                OpStr, {rhsEmit->is_ghost_reference() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) {
          ctx->Error("Binary Operator " + ctx->color(OpStr) + " with right hand side of type " +
                         ctx->color(rhsType->to_string()) + " is a variation of type " +
                         ctx->color(eTy->get_full_name()) +
                         " but the expression on the left hand side does not have variability",
                     fileRange);
        } else if (rhsType->is_reference() && !isVarExp &&
                   eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_reference()->get_subtype()})) {
          ctx->Error("Binary Operator " + ctx->color(OpStr) + " with right hand side of type " +
                         ctx->color(rhsType->as_reference()->get_subtype()->to_string()) + " is a variation of type " +
                         ctx->color(eTy->get_full_name()) +
                         " but the expression on the left hand side does not have variability",
                     fileRange);
        }
        ctx->Error("Matching binary operator " + ctx->color(operator_to_string(op)) + " not found for type " +
                       ctx->color(eTy->get_full_name()) + " with right hand side of type " +
                       ctx->color(rhsType->to_string()),
                   fileRange);
      }
    } else {
      // FIXME - Implement
      ctx->Error("Invalid type for the left hand side of the operator " + ctx->color(operator_to_string(op)) +
                     ". Left hand side is of type " + ctx->color(lhsType->to_string()) +
                     " and the right hand side is of type " + ctx->color(rhsType->to_string()),
                 fileRange);
    }
  }
  return nullptr;
}

Json BinaryExpression::to_json() const {
  return Json()
      ._("nodeType", "binaryExpression")
      ._("operator", operator_to_string(op))
      ._("lhs", lhs->to_json())
      ._("rhs", rhs->to_json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
