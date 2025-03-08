#include "./sub_entity_solver.hpp"
#include "../IR/context.hpp"
#include "../IR/types/array.hpp"
#include "../IR/types/expanded_type.hpp"
#include "../IR/types/integer.hpp"
#include "../IR/types/maybe.hpp"
#include "../IR/types/result.hpp"
#include "../IR/types/struct_type.hpp"
#include "../IR/types/unsigned.hpp"
#include "./emit_ctx.hpp"

namespace qat::ast {

SubEntityResult sub_entity_solver(EmitCtx* ctx, bool isStrictlyPrerun, SubEntityParent parent,
                                  Vec<Identifier> const& names, FileRange fileRange) {
	auto current = parent;
	for (usize i = 0; i < names.size(); i++) {
		auto        isLast     = [&]() { return i == (names.size() - 1); };
		auto        rangeAfter = [&]() { return FileRange{names[i + 1].range, names.back().range}; };
		auto const& name       = names[i];
		if (current.kind == SubEntityParentKind::TYPE) {
			auto type = (ir::Type*)current.data;
			if (type->is_expanded()) {
				auto exp = type->as_expanded();
				if (exp->has_static_method(name.value)) {

					auto statMethod = exp->get_static_method(name.value);
					if (isStrictlyPrerun) {
						ctx->Error("Found a static method named " + ctx->color(statMethod->get_full_name()) +
						               " here. Static methods cannot be present in prerun expressions",
						           name.range);
					}
					if (not isLast()) {
						ctx->Error("Found a static method named " + ctx->color(statMethod->get_full_name()) +
						               " right before this. Static methods do not have children entities",
						           rangeAfter());
					}
					return SubEntityResult::get_expression(statMethod);
				} else if (exp->has_generic_parameter(name.value)) {
					auto gen = exp->get_generic_parameter(name.value);
					if (isLast()) {
						return gen->is_typed() ? SubEntityResult::get_type(gen->as_typed()->get_type())
						                       : SubEntityResult::get_expression(gen->as_prerun()->get_expression());
					} else {
						if (not gen->is_typed()) {
							ctx->Error("Found a prerun generic parameter named " + name.value +
							               " before this. Prerun expressions do not have children entities",
							           rangeAfter());
						}
						current = SubEntityParent::of_type(gen->as_typed()->get_type(), name.range);
						continue;
					}
				} else if (exp->has_definition(name.value)) {
					auto tyDef = exp->get_definition(name.value);
					if (isLast()) {
						return SubEntityResult::get_type(tyDef);
					} else {
						current = SubEntityParent::of_type(tyDef, name.range);
						continue;
					}
				} else if (exp->is_struct()) {
					auto structTy = exp->as_struct();
					if (structTy->has_static_field(name.value)) {
						auto stm = structTy->get_static_field(name.value);
						if (isStrictlyPrerun) {
							if (not stm->has_initial()) {
								ctx->Error(
								    "The static field " + ctx->color(stm->get_full_name()) +
								        " does not have an initial value, so it cannot be used as a prerun expression",
								    name.range);
							}
							if (not stm->get_initial()->is_prerun_value()) {
								ctx->Error("The initial value of the static field " + ctx->color(stm->get_full_name()) +
								               " is not a prerun value and hence cannot be used as a prerun expression",
								           name.range);
							}
							if (stm->is_variable()) {
								ctx->Error(
								    "Found a static field named " + ctx->color(stm->get_full_name()) +
								        " here. But it has variability so its initial value cannot be used as a prerun expression",
								    name.range);
							}
							auto expRes = stm->get_initial()->as_prerun();
							if (not isLast()) {
								ctx->Error("Found a static field named " + ctx->color(stm->get_full_name()) +
								               " before this. Static fields do not have children entities",
								           name.range);
							}
							return SubEntityResult::get_expression(expRes);
						} else {
							if (not isLast()) {
								ctx->Error("Found a static field named " + ctx->color(stm->get_full_name()) +
								               " right before this. Static fields do not have children entities",
								           rangeAfter());
							}
							return SubEntityResult::get_expression(
							    ir::Value::get(stm->get_llvm(), stm->get_ir_type(), stm->is_variable()));
						}
					}
				}
			} else if (type->is_integer()) {
				if (not isLast()) {
					ctx->Error("Found an expression before this. Expressions do not have children entities",
					           rangeAfter());
				}
				if (name.value == "max") {
					return SubEntityResult::get_expression(ir::PrerunValue::get(
					    llvm::ConstantInt::get(type->get_llvm_type(),
					                           llvm::APInt::getSignedMaxValue(type->as_integer()->get_bitwidth())),
					    type));
				} else if (name.value == "min") {
					return SubEntityResult::get_expression(ir::PrerunValue::get(
					    llvm::ConstantInt::get(type->get_llvm_type(),
					                           llvm::APInt::getSignedMinValue(type->as_integer()->get_bitwidth())),
					    type));
				} else if (name.value == "zero") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantInt::get(type->get_llvm_type(), 0u, true), type));
				} else {
					ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
					               ctx->color(type->to_string()),
					           name.range);
				}
			} else if (type->is_unsigned()) {
				if (not isLast()) {
					ctx->Error("Found an expression before this. Expressions do not have children entities",
					           rangeAfter());
				}
				if (name.value == "max") {
					return SubEntityResult::get_expression(ir::PrerunValue::get(
					    llvm::ConstantInt::get(type->get_llvm_type(),
					                           llvm::APInt::getMaxValue(type->as_unsigned()->get_bitwidth())),
					    type));
				} else if (name.value == "min") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantInt::get(type->get_llvm_type(), 0u), type));
				} else if (name.value == "zero") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantInt::get(type->get_llvm_type(), 0u), type));
				} else {
					ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
					               ctx->color(type->to_string()),
					           name.range);
				}
			} else if (type->is_float()) {
				if (not isLast()) {
					ctx->Error("Found an expression before this. Expressions do not have children entities",
					           rangeAfter());
				}
				if (name.value == "infinity") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantFP::getInfinity(type->get_llvm_type(), false), type));
				} else if (name.value == "negative_infinity") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantFP::getInfinity(type->get_llvm_type(), true), type));
				} else if (name.value == "quiet_nan") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantFP::getQNaN(type->get_llvm_type(), false), type));
				} else if (name.value == "signaling_nan") {
					return SubEntityResult::get_expression(
					    ir::PrerunValue::get(llvm::ConstantFP::getSNaN(type->get_llvm_type(), false), type));
				} else if (name.value == "zero" || name.value == "negative_zero") {
					return SubEntityResult::get_expression(ir::PrerunValue::get(
					    llvm::ConstantFP::getZero(type->get_llvm_type(), name.value == "negative_zero"), type));
				} else {
					ctx->Error("Unsupported proprety " + ctx->color(name.value) + " for type " +
					               ctx->color(type->to_string()),
					           name.range);
				}
			} else if (type->is_function()) {
				if (name.value == "given_type") {
					if (isLast()) {
						return SubEntityResult::get_type(type->as_function()->get_return_type()->get_type());
					} else {
						current =
						    SubEntityParent::of_type(type->as_function()->get_return_type()->get_type(), name.range);
						continue;
					}
				} else {
					if (not isLast()) {
						ctx->Error("Found an expression before this. Expressions cannot have children entities",
						           rangeAfter());
					}
					if (name.value == "argument_count") {
						auto uTy = ir::UnsignedType::create(32, ctx->irCtx);
						return SubEntityResult::get_expression(ir::PrerunValue::get(
						    llvm::ConstantInt::get(uTy->get_llvm_type(), type->as_function()->get_argument_count()),
						    uTy));
					} else {
						ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
						               ctx->color(type->to_string()),
						           name.range);
					}
				}
			} else if (type->is_array()) {
				if (name.value == "element_type") {
					if (isLast()) {
						return SubEntityResult::get_type(type->as_array()->get_element_type());
					} else {
						current = SubEntityParent::of_type(type->as_array()->get_element_type(), name.range);
						continue;
					}
				} else {
					if (not isLast()) {
						ctx->Error("Found an expression before this. Expressions do not have children entities",
						           rangeAfter());
					}
					if (name.value == "length") {
						auto uTy = ir::UnsignedType::create(64u, ctx->irCtx);
						return SubEntityResult::get_expression(ir::PrerunValue::get(
						    llvm::ConstantInt::get(uTy->get_llvm_type(), type->as_array()->get_length()), uTy));
					} else {
						ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
						               ctx->color(type->to_string()),
						           name.range);
					}
				}
			} else if (type->is_maybe()) {
				if (name.value == "sub_type") {
					if (isLast()) {
						return SubEntityResult::get_type(type->as_maybe()->get_subtype());
					} else {
						current = SubEntityParent::of_type(type->as_maybe()->get_subtype(), name.range);
						continue;
					}
				} else {
					ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
					               ctx->color(type->to_string()),
					           name.range);
				}
			} else if (type->is_result()) {
				if (name.value == "valid_type") {
					if (isLast()) {
						return SubEntityResult::get_type(type->as_result()->get_valid_type());
					} else {
						current = SubEntityParent::of_type(type->as_result()->get_valid_type(), name.range);
						continue;
					}
				} else if (name.value == "error_type") {
					if (isLast()) {
						return SubEntityResult::get_type(type->as_result()->get_error_type());
					} else {
						current = SubEntityParent::of_type(type->as_result()->get_error_type(), name.range);
						continue;
					}
				} else {
					ctx->Error("Unsupported property " + ctx->color(name.value) + " for type " +
					               ctx->color(type->to_string()),
					           name.range);
				}
			}
		} else if (current.kind == SubEntityParentKind::SKILL) {
			auto* sk = (ir::Skill*)current.data;
			if (sk->has_definition(name.value)) {
				auto def = sk->get_definition(name.value);
				if (isLast()) {
					return SubEntityResult::get_type(def);
				} else {
					current = SubEntityParent::of_type(def, name.range);
					continue;
				}
			} else {
				ctx->Error("Could not find an entity named " + ctx->color(name.value) + " in the skill " +
				               ctx->color(sk->get_full_name()),
				           name.range);
			}
		} else if (current.kind == SubEntityParentKind::DONE_SKILL) {
			auto* sk = (ir::DoneSkill*)current.data;
			if (sk->has_generic_parameter(name.value)) {
				auto gen = sk->get_generic_parameter(name.value);
				if (isLast()) {
					return gen->is_typed() ? SubEntityResult::get_type(gen->as_typed()->get_type())
					                       : SubEntityResult::get_expression(gen->as_prerun()->get_expression());
				} else {
					if (not gen->is_typed()) {
						ctx->Error("Found a prerun generic parameter named " + name.value +
						               " before this. Prerun expressions cannot have children entities",
						           rangeAfter());
					}
					current = SubEntityParent::of_type(gen->as_typed()->get_type(), name.range);
					continue;
				}
			} else if (sk->has_definition(name.value)) {
				auto tyDef = sk->get_definition(name.value);
				if (isLast()) {
					return SubEntityResult::get_type(tyDef);
				} else {
					current = SubEntityParent::of_type(tyDef, name.range);
					continue;
				}
			} else if (sk->has_static_method(name.value)) {
				if (isStrictlyPrerun) {
					ctx->Error("Found the static method named " +
					               ctx->color(sk->get_static_method(name.value)->get_full_name()) +
					               " here. Static methods cannot be used in prerun expressions",
					           name.range);
				}
				return SubEntityResult::get_expression(sk->get_static_method(name.value));
			} else {
				ctx->Error("Could not find an entity named " + ctx->color(name.value) + " in the implementation " +
				               ctx->color(sk->get_full_name()),
				           name.range);
			}
		} else {
			ctx->Error("This entity cannot have children entities", fileRange);
		}
	}
}

} // namespace qat::ast
