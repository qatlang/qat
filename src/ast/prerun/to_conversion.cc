#include "./to_conversion.hpp"

#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/Constants.h>

namespace qat::ast {

void PrerunTo::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
	value->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	targetType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

ir::PrerunValue* PrerunTo::emit(EmitCtx* ctx) {
	auto val		  = value->emit(ctx);
	auto usableTarget = targetType->emit(ctx);
	auto target		  = usableTarget->is_native_type() ? usableTarget->as_native_type()->get_subtype() : usableTarget;
	auto valTy		  = val->get_ir_type();
	if (valTy->is_native_type()) {
		valTy = valTy->as_native_type()->get_subtype();
	}
	if (valTy->is_same(target)) {
		return val;
	}
	if (valTy->is_integer()) {
		if (target->is_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::SExt,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_unsigned_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::ZExt,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_float()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::SIToFP,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		}
	} else if (valTy->is_unsigned_integer()) {
		if (target->is_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::SExt,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_unsigned_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::ZExt,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_float()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::UIToFP,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		}
	} else if (valTy->is_float()) {
		if (target->is_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::FPToSI,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_unsigned_integer()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::FPToUI,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		} else if (target->is_float()) {
			return ir::PrerunValue::get(llvm::ConstantFoldCastInstruction(llvm::Instruction::CastOps::FPExt,
																		  val->get_llvm_constant(),
																		  target->get_llvm_type()),
										usableTarget);
		}
	} else if (valTy->is_string_slice()) {
		if (usableTarget->is_native_type() && usableTarget->as_native_type()->is_cstring()) {
			return ir::PrerunValue::get(val->get_llvm_constant()->getAggregateElement(0u), usableTarget);
		} else if (valTy->is_mark() &&
				   (valTy->as_mark()->get_subtype()->is_unsigned_integer() ||
					(valTy->as_mark()->get_subtype()->is_native_type() &&
					 valTy->as_mark()->get_subtype()->as_native_type()->get_subtype()->is_unsigned_integer())) &&
				   (valTy->as_mark()->get_subtype()->is_unsigned_integer()
						? (valTy->as_mark()->get_subtype()->as_unsigned_integer()->get_bitwidth() == 8u)
						: (valTy->as_mark()
							   ->get_subtype()
							   ->as_native_type()
							   ->get_subtype()
							   ->as_unsigned_integer()
							   ->get_bitwidth() == 8u)) &&
				   (valTy->as_mark()->get_owner().is_of_anonymous()) && (!valTy->as_mark()->is_subtype_variable())) {
			if (valTy->as_mark()->is_slice()) {
				return ir::PrerunValue::get(
					llvm::ConstantExpr::getBitCast(val->get_llvm_constant(), usableTarget->get_llvm_type()),
					usableTarget);
			} else {
				return ir::PrerunValue::get(val->get_llvm_constant()->getAggregateElement(0u), usableTarget);
			}
		}
	} else if (valTy->is_choice()) {
		if ((target->is_integer() || target->is_unsigned_integer()) && valTy->as_choice()->has_provided_type() &&
			(target->is_integer() == valTy->as_choice()->get_provided_type()->is_integer())) {
			auto* intTy = valTy->as_choice()->get_provided_type()->as_integer();
			if (intTy->get_bitwidth() == target->as_integer()->get_bitwidth()) {
				return ir::PrerunValue::get(val->get_llvm(), usableTarget);
			} else {
				ctx->Error("The underlying type of the choice type " + ctx->color(valTy->to_string()) + " is " +
							   ctx->color(intTy->to_string()) + ", but the type for conversion is " +
							   ctx->color(target->to_string()),
						   fileRange);
			}
		} else {
			ctx->Error("The underlying type of the choice type " + ctx->color(valTy->to_string()) + " is " +
						   ctx->color(valTy->as_choice()->get_provided_type()->to_string()) +
						   ", but the target type for conversion is " + ctx->color(target->to_string()),
					   fileRange);
		}
	}
	ctx->Error("Conversion from " + ctx->color(val->get_ir_type()->to_string()) + " to " +
				   ctx->color(usableTarget->to_string()) + " is not supported. Try casting instead",
			   fileRange);
}

String PrerunTo::to_string() const { return value->to_string() + " to " + targetType->to_string(); }

Json PrerunTo::to_json() const {
	return Json()
		._("nodeType", "prerunToConversion")
		._("value", value->to_json())
		._("targetType", targetType->to_json())
		._("fileRange", fileRange);
}

} // namespace qat::ast
