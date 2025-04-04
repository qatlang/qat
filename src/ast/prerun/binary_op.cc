#include "./binary_op.hpp"
#include "../../IR/logic.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/InstrTypes.h>

namespace qat::ast {

ir::PrerunValue* PrerunBinaryOperator::emit(EmitCtx* ctx) {
	ir::PrerunValue* lhsEmit = nullptr;
	ir::PrerunValue* rhsEmit = nullptr;
	if (lhs->nodeType() == NodeType::DEFAULT || lhs->nodeType() == NodeType::NULL_POINTER) {
		rhsEmit = rhs->emit(ctx);
		lhs->as_type_inferrable()->set_inference_type(rhsEmit->get_ir_type());
		lhsEmit = lhs->emit(ctx);
	} else if (rhs->nodeType() == NodeType::DEFAULT || rhs->nodeType() == NodeType::NULL_POINTER) {
		lhsEmit = lhs->emit(ctx);
		rhs->as_type_inferrable()->set_inference_type(lhsEmit->get_ir_type());
		rhsEmit = rhs->emit(ctx);
	} else if (rhs->nodeType() == NodeType::NULL_POINTER) {
		lhsEmit = lhs->emit(ctx);
		rhs->as_type_inferrable()->set_inference_type(lhsEmit->get_ir_type());
		rhsEmit = rhs->emit(ctx);
	} else if ((lhs->nodeType() == NodeType::INTEGER_LITERAL || lhs->nodeType() == NodeType::UNSIGNED_LITERAL ||
	            lhs->nodeType() == NodeType::FLOAT_LITERAL || lhs->nodeType() == NodeType::CUSTOM_FLOAT_LITERAL ||
	            lhs->nodeType() == NodeType::CUSTOM_INTEGER_LITERAL) &&
	           expect_same_operand_types(opr)) {
		rhsEmit = rhs->emit(ctx);
		lhs->as_type_inferrable()->set_inference_type(rhsEmit->get_ir_type());
		lhsEmit = lhs->emit(ctx);
	} else if (rhs->has_type_inferrance() && expect_same_operand_types(opr)) {
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
	auto lhsType  = lhsEmit->get_ir_type();
	auto rhsType  = rhsEmit->get_ir_type();
	auto lhsValTy = lhsType->is_native_type() ? lhsType->as_native_type()->get_subtype() : lhsType;
	auto rhsValTy = rhsType->is_native_type() ? rhsType->as_native_type()->get_subtype() : rhsType;
	if (lhsValTy->is_integer()) {
		if (lhsType->is_same(rhsType)) {
			auto            lhsConst = lhsEmit->get_llvm_constant();
			auto            rhsConst = rhsEmit->get_llvm_constant();
			ir::Type*       resType  = lhsType;
			llvm::Constant* llRes    = nullptr;
			switch (opr) {
				case OperatorKind::ADDITION: {
					llRes = llvm::ConstantExpr::getAdd(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::SUBTRACT: {
					llRes = llvm::ConstantExpr::getSub(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::MULTIPLY: {
					llRes = llvm::ConstantExpr::getMul(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::DIVIDE: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateSDiv(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::REMAINDER: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateSRem(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::EQUAL_TO: {
					llRes = llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::NOT_EQUAL_TO: {
					llRes = llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::BITWISE_AND: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::And, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::BITWISE_OR: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Or, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::BITWISE_XOR: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Xor, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::LOGICAL_LEFT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Shl, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::LOGICAL_RIGHT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::LShr, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::ARITHMETIC_RIGHT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::AShr, lhsConst, rhsConst);
					break;
				}
				default: {
					ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
					               " is not supported for expressions of type " + ctx->color(lhsType->to_string()) +
					               " and " + ctx->color(rhsType->to_string()),
					           fileRange);
				}
			}
			return ir::PrerunValue::get(llvm::ConstantFoldConstant(llRes, ctx->irCtx->dataLayout), resType);
		} else {
			if (rhsValTy->is_integer()) {
				ctx->Error("Signed integers in this binary operation have different bitwidths."
				           " It is recommended to convert the operand with smaller bitwidth to the bigger "
				           "bitwidth to prevent potential loss of data and logical errors",
				           fileRange);
			} else if (rhsValTy->is_unsigned()) {
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
	} else if (lhsValTy->is_unsigned()) {
		if (lhsType->is_same(rhsType)) {
			auto            lhsConst = lhsEmit->get_llvm_constant();
			auto            rhsConst = rhsEmit->get_llvm_constant();
			ir::Type*       resType  = lhsType;
			llvm::Constant* llRes    = nullptr;
			switch (opr) {
				case OperatorKind::ADDITION: {
					llRes = llvm::ConstantExpr::getAdd(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::SUBTRACT: {
					llRes = llvm::ConstantExpr::getSub(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::MULTIPLY: {
					llRes = llvm::ConstantExpr::getMul(lhsConst, rhsConst);
					break;
				}
				case OperatorKind::DIVIDE: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateUDiv(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::REMAINDER: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateURem(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::EQUAL_TO: {
					llRes = llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::NOT_EQUAL_TO: {
					llRes = llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::BITWISE_AND: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::And, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::BITWISE_OR: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Or, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::BITWISE_XOR: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Xor, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::LOGICAL_LEFT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Shl, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::LOGICAL_RIGHT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::LShr, lhsConst, rhsConst);
					break;
				}
				case OperatorKind::ARITHMETIC_RIGHT_SHIFT: {
					llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::AShr, lhsConst, rhsConst);
					break;
				}
				default: {
					ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
					               " is not supported for expressions of type " + ctx->color(lhsType->to_string()) +
					               " and " + ctx->color(rhsType->to_string()),
					           fileRange);
				}
			}
			return ir::PrerunValue::get(llvm::ConstantFoldConstant(llRes, ctx->irCtx->dataLayout), resType);
		} else {
			if (rhsValTy->is_unsigned()) {
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
	} else if (lhsValTy->is_flag() && rhsValTy->is_flag()) {
		if (not lhsValTy->is_same(rhsValTy)) {
			ctx->Error("The flag type on the left hand side is " + ctx->color(lhsValTy->to_string()) +
			               " which does not match with the flag type on the right hand side, which is " +
			               ctx->color(rhsValTy->to_string()),
			           fileRange);
		}
		auto lhsConst = lhsEmit->get_llvm_constant();
		auto rhsConst = lhsEmit->get_llvm_constant();
		if (opr == OperatorKind::EQUAL_TO || opr == OperatorKind::NOT_EQUAL_TO) {
			return ir::PrerunValue::get(llvm::ConstantFoldCompareInstruction((opr == OperatorKind::EQUAL_TO)
			                                                                     ? llvm::CmpInst::ICMP_EQ
			                                                                     : llvm::CmpInst::ICMP_NE,
			                                                                 lhsConst, rhsConst),
			                            ir::UnsignedType::create_bool(ctx->irCtx));
		} else if (opr == OperatorKind::ADDITION) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Or, lhsConst, rhsConst), lhsValTy);
		} else if (opr == OperatorKind::SUBTRACT) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldBinaryInstruction(
			        llvm::Instruction::BinaryOps::Xor,
			        llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Or, lhsConst, rhsConst),
			        rhsConst),
			    lhsValTy);
		} else if (opr == OperatorKind::BITWISE_XOR) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Xor, lhsConst, rhsConst), lhsValTy);
		} else if (opr == OperatorKind::AND) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::And, lhsConst, rhsConst), lhsValTy);
		} else {
			ctx->Error("The " + String(is_unary_operator(opr) ? "unary" : "binary") + " operator " +
			               ctx->color(operator_to_string(opr)) + " is not supported for flag types",
			           fileRange);
		}
	} else if (lhsValTy->is_typed() && rhsValTy->is_typed()) {
		if (opr == OperatorKind::EQUAL_TO || opr == OperatorKind::NOT_EQUAL_TO) {
			return ir::PrerunValue::get(
			    llvm::ConstantInt::get(
			        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			        ir::TypeInfo::get_for(lhsEmit->get_llvm_constant())
			                ->type->is_same(ir::TypeInfo::get_for(rhsEmit->get_llvm_constant())->type)
			            ? ((opr == OperatorKind::EQUAL_TO) ? 1u : 0u)
			            : ((opr == OperatorKind::NOT_EQUAL_TO) ? 1u : 0u)),
			    ir::UnsignedType::create_bool(ctx->irCtx));
		} else {
			ctx->Error("The only operators allowed for comparing Type IDs are " + ctx->color("==") + " and " +
			               ctx->color("!="),
			           fileRange);
		}
	} else if (lhsValTy->is_bool() && rhsValTy->is_bool()) {
		auto            lhsConst = lhsEmit->get_llvm_constant();
		auto            rhsConst = rhsEmit->get_llvm_constant();
		llvm::Constant* llRes    = nullptr;
		switch (opr) {
			case OperatorKind::OR: {
				llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::Or, lhsConst, rhsConst);
				break;
			}
			case OperatorKind::AND: {
				llRes = llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::And, lhsConst, rhsConst);
				break;
			}
			case OperatorKind::EQUAL_TO: {
				llRes = llvm::ConstantFoldCompareInstruction(llvm::ICmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst);
				break;
			}
			case OperatorKind::NOT_EQUAL_TO: {
				llRes = llvm::ConstantFoldCompareInstruction(llvm::ICmpInst::Predicate::ICMP_NE, lhsConst, rhsConst);
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
				case OperatorKind::ADDITION: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFAdd(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::SUBTRACT: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFSub(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::MULTIPLY: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFMul(lhsConst, rhsConst));
					break;
				}
				case OperatorKind::DIVIDE: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFDiv(lhsConst, rhsConst));
					// ctx->getMod()->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
					break;
				}
				case OperatorKind::REMAINDER: {
					llRes = llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFRem(lhsConst, rhsConst));
					// ctx->getMod()->nativeLibsToLink.push_back(ir::LibToLink::fromName({"m", fileRange}, fileRange));
					break;
				}
				case OperatorKind::EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_OEQ, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::NOT_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_ONE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_OLT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_OGT, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::LESS_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_OLE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				case OperatorKind::GREATER_THAN_OR_EQUAL_TO: {
					llRes =
					    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::FCMP_OGE, lhsConst, rhsConst);
					resType = ir::UnsignedType::create_bool(ctx->irCtx);
					break;
				}
				default: {
					ctx->Error("The operator " + ctx->color(operator_to_string(opr)) +
					               " is not supported for expressions of type " + ctx->color(lhsType->to_string()) +
					               " and " + ctx->color(rhsType->to_string()),
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
	} else if (lhsType->is_text() && rhsType->is_text()) {
		if (opr == OperatorKind::EQUAL_TO) {
			return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			                                                   lhsEmit->is_equal_to(ctx->irCtx, rhsEmit) ? 1u : 0u),
			                            ir::UnsignedType::create_bool(ctx->irCtx));
		} else if (opr == OperatorKind::NOT_EQUAL_TO) {
			return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			                                                   lhsEmit->is_equal_to(ctx->irCtx, rhsEmit) ? 0u : 1u),
			                            ir::UnsignedType::create_bool(ctx->irCtx));
		} else {
			ctx->Error("Unsupported binary operator " + ctx->color(operator_to_string(opr)) + " for operands of type " +
			               ctx->color(lhsType->to_string()),
			           fileRange);
		}
	} else if (lhsType->is_choice() && lhsType->is_same(rhsType)) {
		auto chTy     = lhsType->as_choice();
		auto lhsConst = lhsEmit->get_llvm_constant();
		auto rhsConst = rhsEmit->get_llvm_constant();
		if (opr == OperatorKind::EQUAL_TO) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ, lhsConst, rhsConst),
			    ir::UnsignedType::create_bool(ctx->irCtx));
		} else if (opr == OperatorKind::NOT_EQUAL_TO) {
			return ir::PrerunValue::get(
			    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_NE, lhsConst, rhsConst),
			    ir::UnsignedType::create_bool(ctx->irCtx));
		} else if (opr == OperatorKind::LESS_THAN) {
			if (chTy->has_negative_values()) {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SLT, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			} else {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_ULT, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			}
		} else if (opr == OperatorKind::LESS_THAN_OR_EQUAL_TO) {
			if (chTy->has_negative_values()) {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SLE, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			} else {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_ULE, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			}
		} else if (opr == OperatorKind::GREATER_THAN) {
			if (chTy->has_negative_values()) {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SGT, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			} else {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_UGT, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			}
		} else if (opr == OperatorKind::GREATER_THAN_OR_EQUAL_TO) {
			if (chTy->has_negative_values()) {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_SGE, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			} else {
				return ir::PrerunValue::get(
				    llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_UGE, lhsConst, rhsConst),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			}
		} else {
			ctx->Error("Binary operator " + ctx->color(operator_to_string(opr)) +
			               " is not supported for expressions of type " + ctx->color(lhsType->to_string()),
			           fileRange);
		}
	} else if (lhsValTy->is_ptr() && rhsValTy->is_ptr()) {
		if (lhsValTy->as_ptr()->get_subtype()->is_same(rhsValTy->as_ptr()->get_subtype()) &&
		    (lhsValTy->as_ptr()->is_multi() == rhsValTy->as_ptr()->is_multi())) {
			llvm::Constant* finalCondition = nullptr;
			if (opr != OperatorKind::EQUAL_TO && opr != OperatorKind::NOT_EQUAL_TO) {
				ctx->Error("Unsupported operator " + ctx->color(operator_to_string(opr)) + " for expressions of type " +
				               ctx->color(lhsType->to_string()) + " and " + ctx->color(rhsType->to_string()),
				           fileRange);
			}
			finalCondition = llvm::ConstantFoldCompareInstruction(
			    (opr == OperatorKind::EQUAL_TO) ? llvm::CmpInst::Predicate::ICMP_EQ : llvm::CmpInst::Predicate::ICMP_NE,
			    llvm::ConstantExpr::getSub(
			        llvm::ConstantExpr::getPtrToInt(
			            lhsValTy->as_ptr()->is_multi() ? lhsEmit->get_llvm()->getAggregateElement(0u)
			                                           : lhsEmit->get_llvm(),
			            llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getPointerSizeInBits(
			                                                         ctx->irCtx->dataLayout.getProgramAddressSpace()))),
			        llvm::ConstantExpr::getPtrToInt(
			            lhsValTy->as_ptr()->is_multi() ? rhsEmit->get_llvm()->getAggregateElement(0u)
			                                           : rhsEmit->get_llvm(),
			            llvm::Type::getIntNTy(ctx->irCtx->llctx,
			                                  ctx->irCtx->dataLayout.getPointerSizeInBits(
			                                      ctx->irCtx->dataLayout.getProgramAddressSpace())))),
			    llvm::ConstantInt::get(
			        llvm::Type::getIntNTy(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getPointerSizeInBits(
			                                                     ctx->irCtx->dataLayout.getProgramAddressSpace())),
			        0u));
			if (lhsValTy->as_ptr()->is_multi()) {
				finalCondition = llvm::ConstantFoldBinaryInstruction(
				    (opr == OperatorKind::EQUAL_TO) ? llvm::Instruction::BinaryOps::And
				                                    : llvm::Instruction::BinaryOps::Or,
				    llvm::ConstantFoldCompareInstruction(
				        (opr == OperatorKind::EQUAL_TO) ? llvm::CmpInst::Predicate::ICMP_EQ
				                                        : llvm::CmpInst::Predicate::ICMP_NE,
				        lhsEmit->get_llvm()->getAggregateElement(1u), rhsEmit->get_llvm()->getAggregateElement(1u)),
				    finalCondition);
			}
			return ir::PrerunValue::get(llvm::ConstantFoldConstant(finalCondition, ctx->irCtx->dataLayout),
			                            ir::UnsignedType::create_bool(ctx->irCtx));
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

String PrerunBinaryOperator::to_string() const {
	return lhs->to_string() + " " + operator_to_string(opr) + " " + rhs->to_string();
}

Json PrerunBinaryOperator::to_json() const {
	return Json()
	    ._("nodeType", "prerunBinaryOp")
	    ._("lhs", lhs->to_json())
	    ._("operator", operator_to_string(opr))
	    ._("rhs", rhs->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
