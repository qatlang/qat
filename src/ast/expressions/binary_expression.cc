#include "binary_expression.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/reference.hpp"
#include "operator.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>

#define QAT_COMPARISON_INDEX "qat'str'comparisonIndex"

namespace qat::ast {

ir::Value* BinaryExpression::emit(EmitCtx* ctx) {
	SHOW("Lhs node type is " << (int)lhs->nodeType() << " and rhs node type is " << (int)rhs->nodeType())
	ir::Value* lhsEmit = nullptr;
	ir::Value* rhsEmit = nullptr;
	if (lhs->nodeType() == NodeType::DEFAULT) {
		rhsEmit = rhs->emit(ctx);
		lhs->as_type_inferrable()->set_inference_type(
		    rhsEmit->is_ref() ? rhsEmit->get_ir_type()->as_ref()->get_subtype() : rhsEmit->get_ir_type());
		lhsEmit = lhs->emit(ctx);
	} else if (rhs->nodeType() == NodeType::DEFAULT) {
		lhsEmit = lhs->emit(ctx);
		rhs->as_type_inferrable()->set_inference_type(
		    lhsEmit->is_ref() ? lhsEmit->get_ir_type()->as_ref()->get_subtype() : lhsEmit->get_ir_type());
		rhsEmit = rhs->emit(ctx);
	} else if (lhs->nodeType() == NodeType::NULL_MARK) {
		rhsEmit = rhs->emit(ctx);
		lhs->as_type_inferrable()->set_inference_type(rhsEmit->get_ir_type());
		lhsEmit = lhs->emit(ctx);
	} else if (rhs->nodeType() == NodeType::NULL_MARK) {
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
		lhsEmit = lhs->emit(ctx);
		auto lhsTy =
		    lhsEmit->get_ir_type()->is_ref() ? lhsEmit->get_ir_type()->as_ref()->get_subtype() : lhsEmit->get_ir_type();
		if (lhsTy->is_native_type()) {
			lhsTy = lhsTy->as_native_type()->get_subtype();
		}
		if (lhsTy->is_integer() || lhsTy->is_unsigned() || lhsTy->is_float()) {
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
        lhsEmit->load_ghost_ref(ctx->irCtx->builder);
        SHOW("Loaded LHS")
        lhsVal = lhsEmit->get_llvm();
        SHOW("Loading implicit RHS")
        rhsEmit->load_ghost_ref(ctx->irCtx->builder);
        SHOW("Loaded RHS")
        rhsVal = rhsEmit->get_llvm();
        if (lhsEmit->is_ref()) {
            SHOW("LHS is reference")
            lhsType = lhsType->as_ref()->get_subtype();
            SHOW("LHS type is: " << lhsType->to_string())
            lhsVal = ctx->irCtx->builder.CreateLoad(lhsType->get_llvm_type(), lhsVal, false);
        }
        if (rhsEmit->is_ref()) {
            SHOW("RHS is reference")
            rhsType = rhsType->as_ref()->get_subtype();
            SHOW("RHS type is: " << rhsType->to_string())
            rhsVal = ctx->irCtx->builder.CreateLoad(rhsType->get_llvm_type(), rhsVal, false);
        }
	};
	auto lhsValueType = lhsType->is_ref() ? lhsType->as_ref()->get_subtype() : lhsType;
	if (lhsValueType->is_native_type()) {
		lhsValueType = lhsValueType->as_native_type()->get_subtype();
	}
	auto rhsValueType = rhsType->is_ref() ? rhsType->as_ref()->get_subtype() : rhsType;
	if (rhsValueType->is_native_type()) {
		rhsValueType = rhsValueType->as_native_type()->get_subtype();
	}
	if (lhsValueType->is_bool() && rhsValueType->is_bool()) {
		referenceHandler();
		if (op == Op::equalTo) {
			return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false)
			    ->with_range(fileRange);
		} else if (op == Op::notEqualTo) {
			return ir::Value::get(ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false)
			    ->with_range(fileRange);
		} else if (op == Op::And) {
			return ir::Value::get(ctx->irCtx->builder.CreateLogicalAnd(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false)
			    ->with_range(fileRange);
		} else if (op == Op::Or) {
			return ir::Value::get(ctx->irCtx->builder.CreateLogicalOr(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false);
		} else {
			ctx->Error("Unsupported operator " + ctx->color(operator_to_string(op)) + " for expressions of type " +
			               ctx->color(lhsType->to_string()),
			           fileRange);
		}
	} else if (lhsValueType->is_integer()) {
		referenceHandler();
		SHOW("Integer binary operation: " << operator_to_string(op))
		if (lhsType->is_same(rhsType) ||
		    (not lhsType->is_native_type() && not rhsType->is_native_type() && lhsValueType->is_same(rhsValueType))) {
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
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::notEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThan: {
					llRes   = ctx->irCtx->builder.CreateICmpSLT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThan: {
					llRes   = ctx->irCtx->builder.CreateICmpSGT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThanOrEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpSLE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThanEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpSGE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
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
			if (rhsValueType->is_integer()) {
				ctx->Error("Signed integers in this binary operation have different bitwidths."
				           " It is recommended to convert the operand with smaller bitwidth to the bigger "
				           "bitwidth to prevent potential loss of data and logical errors",
				           fileRange);
			} else if (rhsValueType->is_unsigned()) {
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
	} else if (lhsValueType->is_unsigned()) {
		SHOW("Unsigned integer binary operation")
		referenceHandler();
		if (lhsType->is_same(rhsType) ||
		    (not lhsType->is_native_type() && not rhsType->is_native_type() && lhsValueType->is_same(rhsValueType))) {
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
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::notEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThan: {
					llRes   = ctx->irCtx->builder.CreateICmpULT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThan: {
					llRes   = ctx->irCtx->builder.CreateICmpUGT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThanOrEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpULE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThanEqualTo: {
					llRes   = ctx->irCtx->builder.CreateICmpUGE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
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
					               " is not supported for expressions of type " + ctx->color(lhsType->to_string()) +
					               " and " + ctx->color(rhsType->to_string()),
					           fileRange);
				}
			}
			// NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
			return ir::Value::get(llRes, resType, false)->with_range(fileRange);
		} else {
			if (rhsValueType->is_unsigned()) {
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
		    (not lhsType->is_native_type() && not rhsType->is_native_type() && lhsValueType->is_same(rhsValueType))) {
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
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::notEqualTo: {
					llRes   = ctx->irCtx->builder.CreateFCmpONE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThan: {
					llRes   = ctx->irCtx->builder.CreateFCmpOLT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThan: {
					llRes   = ctx->irCtx->builder.CreateFCmpOGT(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::lessThanOrEqualTo: {
					llRes   = ctx->irCtx->builder.CreateFCmpOLE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case Op::greaterThanEqualTo: {
					llRes   = ctx->irCtx->builder.CreateFCmpOGE(lhsVal, rhsVal);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				default: {
					ctx->Error("The operator " + ctx->color(operator_to_string(op)) +
					               " is not supported for expressions of type " + ctx->color(lhsType->to_string()) +
					               " and " + ctx->color(rhsType->to_string()),
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
			} else if (rhsType->is_unsigned()) {
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
	} else if (lhsValueType->is_text() && rhsValueType->is_text()) {
		if (op == OperatorKind::EQUAL_TO || op == OperatorKind::NOT_EQUAL_TO) {
			return ir::Logic::compare_text(op == OperatorKind::EQUAL_TO, lhsEmit, rhsEmit, lhs->fileRange,
			                               rhs->fileRange, fileRange, ctx);
		} else {
			ctx->Error(ctx->color("text") + " type does not support the " + ctx->color(operator_to_string(op)) +
			               " operator",
			           fileRange);
		}
			if (lhsEmit->is_llvm_constant()) {
			} else {
			}
			if (rhsEmit->is_llvm_constant()) {
			} else {
			}
			return ir::Logic::compare_text(op == OperatorKind::EQUAL_TO, lhsStr, rhsStr, lhs->fileRange, rhs->fileRange,
			                               fileRange, ctx);
		} else {
			ctx->Error("The only supported operators for comparing Type IDs are " + ctx->color("==") + " and " +
			               ctx->color("!="),
			           fileRange);
		}
	} else if (lhsValueType->is_mark() && rhsValueType->is_mark()) {
		SHOW("LHS type is: " << lhsType->to_string() << " and RHS type is: " << rhsType->to_string())
		if (lhsValueType->as_mark()->get_subtype()->is_same(rhsValueType->as_mark()->get_subtype()) &&
		    (lhsValueType->as_mark()->is_slice() == rhsValueType->as_mark()->is_slice())) {
			if (lhsValueType->as_mark()->is_slice()) {
				bool isLHSRef = false;
				SHOW("LHS side")
				if (lhsEmit->is_ref()) {
					lhsEmit->load_ghost_ref(ctx->irCtx->builder);
					isLHSRef = true;
				} else if (lhsEmit->is_ghost_ref()) {
					isLHSRef = true;
				}
				auto* ptrType = lhsValueType->as_mark();
				lhsType       = ptrType;
				auto resPtrTy = ir::MarkType::get(false, ptrType->get_subtype(), ptrType->is_non_nullable(),
				                                  ir::MarkOwner::of_anonymous(), false, ctx->irCtx);
				if (lhsEmit->is_prerun_value()) {
					lhsEmit = ir::PrerunValue::get(lhsEmit->get_llvm_constant()->getAggregateElement(0u), resPtrTy);
				} else if (isLHSRef) {
					lhsEmit = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(
					        resPtrTy->get_llvm_type(),
					        ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), lhsEmit->get_llvm(), 0u)),
					    resPtrTy, false);
				} else {
					lhsEmit = ir::Value::get(ctx->irCtx->builder.CreateExtractValue(lhsEmit->get_llvm(), {0u}),
					                         resPtrTy, false);
				}
				lhsVal = lhsEmit->get_llvm();
				SHOW("Set LhsEmit")
			} else if (lhsEmit->is_ref() || lhsEmit->is_ghost_ref()) {
				lhsEmit->load_ghost_ref(ctx->irCtx->builder);
				if (lhsEmit->is_ref()) {
					lhsEmit = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(lhsValueType->get_llvm_type(), lhsEmit->get_llvm()),
					    lhsValueType, false);
				}
				lhsType = lhsValueType;
				lhsVal  = lhsEmit->get_llvm();
			}
			if (rhsValueType->as_mark()->is_slice()) {
				bool isRHSRef = false;
				SHOW("RHS side")
				if (rhsEmit->is_ref()) {
					SHOW("Loading RHS")
					rhsEmit->load_ghost_ref(ctx->irCtx->builder);
					isRHSRef = true;
				} else if (rhsEmit->is_ghost_ref()) {
					isRHSRef = true;
				}
				auto* ptrType = rhsValueType->as_mark();
				rhsType       = ptrType;
				auto resPtrTy = ir::MarkType::get(false, ptrType->get_subtype(), ptrType->is_non_nullable(),
				                                  ir::MarkOwner::of_anonymous(), false, ctx->irCtx);
				if (rhsEmit->is_prerun_value()) {
					rhsEmit = ir::PrerunValue::get(rhsEmit->get_llvm_constant()->getAggregateElement(0u), resPtrTy);
				} else if (isRHSRef) {
					rhsEmit = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(
					        resPtrTy->get_llvm_type(),
					        ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), rhsEmit->get_llvm(), 0u)),
					    resPtrTy, false);
				} else {
					rhsEmit = ir::Value::get(ctx->irCtx->builder.CreateExtractValue(rhsEmit->get_llvm(), {0u}),
					                         resPtrTy, false);
				}
				rhsVal = rhsEmit->get_llvm();
				SHOW("Set RhsEmit")
			} else if (rhsEmit->is_ref() || rhsEmit->is_ghost_ref()) {
				rhsEmit->load_ghost_ref(ctx->irCtx->builder);
				if (rhsEmit->is_ref()) {
					rhsEmit = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(rhsValueType->get_llvm_type(), rhsEmit->get_llvm()),
					    rhsValueType, false);
				}
				rhsType = rhsValueType;
				rhsVal  = rhsEmit->get_llvm();
			}
			SHOW("LHS type is: " << lhsType->to_string() << " and RHS type is: " << rhsType->to_string())
			auto ptrTy = lhsValueType->as_mark();
			if (op == Op::equalTo) {
				SHOW("Pointer is normal")
				return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(
				                          ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
				                                                            lhsVal, rhsVal),
				                          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			} else if (op == Op::notEqualTo) {
				SHOW("Pointer is normal")
				return ir::Value::get(ctx->irCtx->builder.CreateICmpNE(
				                          ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
				                                                            lhsVal, rhsVal),
				                          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
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
			return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false)
			    ->with_range(fileRange);
		} else if (op == Op::notEqualTo) {
			return ir::Value::get(ctx->irCtx->builder.CreateICmpNE(lhsVal, rhsVal),
			                      ir::UnsignedType::create_bool(ctx->irCtx), false)
			    ->with_range(fileRange);
		} else if (op == Op::lessThan) {
			if (chTy->has_negative_values()) {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpSLT(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			} else {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpULT(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			}
		} else if (op == Op::lessThanOrEqualTo) {
			if (chTy->has_negative_values()) {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpSLE(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			} else {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpULE(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			}
		} else if (op == Op::greaterThan) {
			if (chTy->has_negative_values()) {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpSGT(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			} else {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpUGT(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			}
		} else if (op == Op::greaterThanEqualTo) {
			if (chTy->has_negative_values()) {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpSGE(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			} else {
				return ir::Value::get(ctx->irCtx->builder.CreateICmpUGE(lhsVal, rhsVal),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false)
				    ->with_range(fileRange);
			}
		} else {
			ctx->Error("This binary operator is not supported for expressions of type " +
			               ctx->color(lhsType->to_string()),
			           fileRange);
		}
	} else {
		if (lhsType->is_expanded() || (lhsType->is_ref() && lhsType->as_ref()->get_subtype()->is_expanded())) {
			SHOW("Expanded type binary operation")
			auto* eTy   = lhsType->is_ref() ? lhsType->as_ref()->get_subtype()->as_expanded() : lhsType->as_expanded();
			auto  OpStr = operator_to_string(op);
			bool  isVarExp = lhsType->is_ref() ? lhsType->as_ref()->has_variability()
			                                   : (lhsEmit->is_ghost_ref() ? lhsEmit->is_variable() : true);
			if ((isVarExp &&
			     eTy->has_variation_binary_operator(
			         OpStr, {rhsEmit->is_ghost_ref() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) ||
			    eTy->has_normal_binary_operator(
			        OpStr, {rhsEmit->is_ghost_ref() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) {
				SHOW("RHS is matched exactly")
				auto localID = lhsEmit->get_local_id();
				if (not lhsType->is_ref() && not lhsEmit->is_ghost_ref()) {
					lhsEmit = lhsEmit->make_local(ctx, None, lhs->fileRange);
				} else if (lhsType->is_ref()) {
					lhsEmit->load_ghost_ref(ctx->irCtx->builder);
				}
				auto* opFn =
				    (isVarExp &&
				     eTy->has_variation_binary_operator(
				         OpStr, {rhsEmit->is_ghost_ref() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType}))
				        ? eTy->get_variation_binary_operator(
				              OpStr, {lhsEmit->is_ghost_ref() ? Maybe<bool>(lhsEmit->is_variable()) : None, rhsType})
				        : eTy->get_normal_binary_operator(
				              OpStr, {lhsEmit->is_ghost_ref() ? Maybe<bool>(lhsEmit->is_variable()) : None, rhsType});
				if (not opFn->is_accessible(ctx->get_access_info())) {
					ctx->Error(String(isVarExp ? "Variation b" : "B") + "inary operator " +
					               ctx->color(operator_to_string(op)) + " of type " + ctx->color(eTy->get_full_name()) +
					               " with right hand side type " + ctx->color(rhsType->to_string()) +
					               " is not accessible here",
					           fileRange);
				}
				rhsEmit->load_ghost_ref(ctx->irCtx->builder);
				return opFn->call(ctx->irCtx, {lhsEmit->get_llvm(), rhsEmit->get_llvm()}, localID, ctx->mod)
				    ->with_range(fileRange);
			} else if (rhsType->is_ref() &&
			           ((isVarExp &&
			             eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()})) ||
			            eTy->has_normal_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()}))) {
				auto localID = rhsEmit->get_local_id();
				rhsEmit->load_ghost_ref(ctx->irCtx->builder);
				SHOW("RHS is matched as subtype of reference")
				if (not lhsType->is_ref() && not lhsEmit->is_ghost_ref()) {
					lhsEmit = lhsEmit->make_local(ctx, None, lhs->fileRange);
				}
				auto* opFn =
				    (isVarExp && eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()}))
				        ? eTy->get_variation_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()})
				        : eTy->get_normal_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()});
				if (not opFn->is_accessible(ctx->get_access_info())) {
					ctx->Error("Operator " + ctx->color(operator_to_string(op)) + " of type " +
					               ctx->color(eTy->get_full_name()) + " with right hand side type: " +
					               ctx->color(rhsType->to_string()) + " is not accessible here",
					           fileRange);
				}
				return opFn
				    ->call(ctx->irCtx,
				           {lhsEmit->get_llvm(),
				            ctx->irCtx->builder.CreateLoad(rhsType->as_ref()->get_subtype()->get_llvm_type(),
				                                           rhsEmit->get_llvm())},
				           localID, ctx->mod)
				    ->with_range(fileRange);
			} else {
				if (not isVarExp &&
				    eTy->has_variation_binary_operator(
				        OpStr, {rhsEmit->is_ghost_ref() ? Maybe<bool>(rhsEmit->is_variable()) : None, rhsType})) {
					ctx->Error("Binary Operator " + ctx->color(OpStr) + " with right hand side of type " +
					               ctx->color(rhsType->to_string()) + " is a variation of type " +
					               ctx->color(eTy->get_full_name()) +
					               " but the expression on the left hand side does not have variability",
					           fileRange);
				} else if (rhsType->is_ref() && not isVarExp &&
				           eTy->has_variation_binary_operator(OpStr, {None, rhsType->as_ref()->get_subtype()})) {
					ctx->Error("Binary Operator " + ctx->color(OpStr) + " with right hand side of type " +
					               ctx->color(rhsType->as_ref()->get_subtype()->to_string()) +
					               " is a variation of type " + ctx->color(eTy->get_full_name()) +
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
