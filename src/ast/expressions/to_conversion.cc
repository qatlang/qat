#include "./to_conversion.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/integer.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/text.hpp"
#include "../../IR/types/unsigned.hpp"

#include <llvm/IR/Instructions.h>

namespace qat::ast {

ir::Value* ToConversion::emit(EmitCtx* ctx) {
	auto* val    = source->emit(ctx);
	auto* destTy = destinationType->emit(ctx);
	if (val->get_ir_type()->is_same(destTy)) {
		if (val->get_ir_type()->is_type_definition() || destTy->is_type_definition()) {
			return ir::Value::get(val->get_llvm(), destTy, false);
		} else {
			ctx->irCtx->Warning("Unnecessary conversion here. The expression is already of type " +
			                        ctx->irCtx->highlightWarning(destTy->to_string()) +
			                        ". Please remove this to avoid clutter.",
			                    fileRange);
			return val;
		}
	} else {
		auto* typ     = val->get_ir_type();
		auto  loadRef = [&] {
            if (val->is_ref()) {
                val->load_ghost_ref(ctx->irCtx->builder);
                val = ir::Value::get(ctx->irCtx->builder.CreateLoad(
                                         val->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(), val->get_llvm()),
				                      val->get_ir_type()->as_ref()->get_subtype(), false);
                typ = val->get_ir_type();
            } else {
                val->load_ghost_ref(ctx->irCtx->builder);
            }
		};
		auto valType = val->is_ref() ? val->get_ir_type()->as_ref()->get_subtype() : val->get_ir_type();
		if (valType->is_native_type()) {
			valType = valType->as_native_type()->get_subtype();
		}
		if (valType->is_ptr()) {
			auto valPtrTy = valType->as_ptr();
			if (destTy->is_ptr()) {
				loadRef();
				auto targetTy =
				    destTy->is_native_type() ? destTy->as_native_type()->get_subtype()->as_ptr() : destTy->as_ptr();
				if (not(valPtrTy->get_locality().is_same(targetTy->get_locality())) and
				    not(targetTy->get_locality().is_none())) {
					ctx->Error(
					    "This change of locality of the pointer type is not allowed. Pointers with known locality can only be converted to anonymous locality.",
					    fileRange);
				}
				if (valPtrTy->is_nullable() != targetTy->as_ptr()->is_nullable()) {
					if (valPtrTy->is_nullable()) {
						auto  fun           = ctx->get_fn();
						auto* currBlock     = fun->get_block();
						auto* nullTrueBlock = ir::Block::create(fun, currBlock);
						auto* restBlock     = ir::Block::create(fun, currBlock->get_parent());
						restBlock->link_previous_block(currBlock);
						auto intPtrTy = llvm::Type::getIntNTy(
						    ctx->irCtx->llctx, ctx->irCtx->dataLayout.getPointerTypeSizeInBits(llvm::PointerType::get(
						                           ctx->irCtx->llctx, valPtrTy->usable_address_space(ctx->irCtx))));
						ctx->irCtx->builder.CreateCondBr(
						    ctx->irCtx->builder.CreateICmpEQ(
						        ctx->irCtx->builder.CreatePtrToInt(
						            valPtrTy->is_multi() ? ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u})
						                                 : val->get_llvm(),
						            intPtrTy),
						        llvm::ConstantInt::get(intPtrTy, 0u, false)),
						    nullTrueBlock->get_bb(), restBlock->get_bb());
						nullTrueBlock->set_active(ctx->irCtx->builder);
						ir::Logic::panic_in_function(
						    fun,
						    {ir::TextType::create_value(
						        ctx->irCtx, ctx->mod,
						        "This is a null-pointer and hence cannot be converted to the non-nullable pointer type " +
						            destTy->to_string())},
						    {}, fileRange, ctx);
						(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
						restBlock->set_active(ctx->irCtx->builder);
					}
				}
				if (not valPtrTy->get_subtype()->is_same(targetTy->get_subtype())) {
					ctx->Error(
					    "The value to be converted is of type " + ctx->color(typ->to_string()) +
					        " but the destination type is " + ctx->color(destTy->to_string()) +
					        ". The subtype of the pointer types do not match and conversion between them is not allowed. Use casting instead",
					    fileRange);
				}
				if (valPtrTy->is_multi() != targetTy->as_ptr()->is_multi()) {
					if (valPtrTy->is_multi()) {
						return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy,
						                      false);
					} else {
						auto newVal = ctx->get_fn()->get_block()->new_local(utils::uid_string(), destTy, false,
						                                                    ctx->irCtx, fileRange);
						ctx->irCtx->builder.CreateStore(
						    val->get_llvm(),
						    ctx->irCtx->builder.CreateStructGEP(destTy->get_llvm_type(), newVal->get_llvm(), 0u));
						ctx->irCtx->builder.CreateStore(
						    llvm::ConstantInt::get(ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(), 1u),
						    ctx->irCtx->builder.CreateStructGEP(destTy->get_llvm_type(), newVal->get_llvm(), 1u));
						return newVal->to_new_ir_value();
					}
				}
				if (valPtrTy->is_multi()) {
					val->get_llvm()->mutateType(destTy->get_llvm_type());
					return ir::Value::get(val->get_llvm(), destTy, false);
				} else {
					return ir::Value::get(
					    ctx->irCtx->builder.CreatePointerCast(val->get_llvm(), destTy->get_llvm_type()), destTy, false);
				}
			} else if (destTy->is_native_type() && (destTy->as_native_type()->is_native_intptr() ||
			                                        destTy->as_native_type()->is_native_uintptr())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateBitCast(
				                          valPtrTy->is_multi()
				                              ? ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u})
				                              : val->get_llvm(),
				                          destTy->get_llvm_type()),
				                      destTy, false);
			} else {
				ctx->Error("Pointer conversion to type " + ctx->color(destTy->to_string()) + " is not supported",
				           fileRange);
			}
		} else if (valType->is_text()) {
			auto destValTy = destTy->is_native_type() ? destTy->as_native_type()->get_subtype() : destTy;
			if (destTy->is_native_type() && destTy->as_native_type()->is_native_byteptr()) {
				if (val->is_prerun_value()) {
					return ir::PrerunValue::get(val->get_llvm_constant()->getAggregateElement(0u), destTy);
				} else {
					loadRef();
					return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy, false);
				}
			} else if (destValTy->is_ptr() and
			           (destValTy->as_ptr()->get_subtype()->is_unsigned() or
			            (destValTy->as_ptr()->get_subtype()->is_native_type() and
			             destValTy->as_ptr()->get_subtype()->as_native_type()->get_subtype()->is_unsigned())) and
			           destValTy->as_ptr()->get_locality().is_none() and
			           (destValTy->as_ptr()->get_subtype()->is_unsigned()
			                ? (destValTy->as_ptr()->get_subtype()->as_unsigned()->get_bitwidth() == 8u)
			                : (destValTy->as_ptr()
			                       ->get_subtype()
			                       ->as_native_type()
			                       ->get_subtype()
			                       ->as_unsigned()
			                       ->get_bitwidth() == 8u)) and
			           not(destValTy->as_ptr()->is_subtype_variable())) {
				loadRef();
				if (destValTy->as_ptr()->is_multi()) {
					val->get_llvm()->mutateType(destTy->get_llvm_type());
					return ir::Value::get(val->get_llvm(), destTy, false);
				} else {
					return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy, false);
				}
			} else {
				ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " +
				               ctx->color(destTy->to_string()) + " is not supported",
				           fileRange);
			}
		} else if (valType->is_integer()) {
			if (destTy->is_integer() ||
			    (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_integer())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true),
				                      destTy, val->has_variability());
			} else if (destTy->is_unsigned() ||
			           (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_unsigned())) {
				loadRef();
				ctx->irCtx->Warning("Conversion from signed integer to unsigned integers can be lossy", fileRange);
				if (valType->as_integer()->get_bitwidth() == destTy->as_unsigned()->get_bitwidth()) {
					return ir::Value::get(
					    ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true), destTy,
					    false);
				} else {
					return ir::Value::get(
					    ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true), destTy,
					    false);
				}
			} else if (destTy->is_float() || (destTy->is_native_type() && destTy->as_native_type()->is_float())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateSIToFP(val->get_llvm(), destTy->get_llvm_type()),
				                      destTy, false);
			} else {
				ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " +
				               ctx->color(destTy->to_string()) + " is not supported",
				           fileRange);
			}
		} else if (valType->is_unsigned()) {
			if (destTy->is_unsigned() ||
			    (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_unsigned())) {
				loadRef();
				return ir::Value::get(
				    ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), false), destTy, false);
			} else if (destTy->is_integer() ||
			           (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_integer())) {
				loadRef();
				ctx->irCtx->Warning("Conversion from unsigned integer to signed integers can be lossy", fileRange);
				if (typ->as_unsigned()->get_bitwidth() == destTy->as_integer()->get_bitwidth()) {
					return ir::Value::get(val->get_llvm(), destTy, false);
				} else {
					return ir::Value::get(
					    ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true), destTy,
					    false);
				}
			} else if (destTy->is_float() ||
			           (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_float())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateUIToFP(val->get_llvm(), destTy->get_llvm_type()),
				                      destTy, false);
			} else {
				ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " +
				               ctx->color(destTy->to_string()) + " is not supported",
				           fileRange);
			}
		} else if (valType->is_float()) {
			if (destTy->is_float() ||
			    (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_float())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateFPCast(val->get_llvm(), destTy->get_llvm_type()),
				                      destTy, false);
			} else if (destTy->is_integer() ||
			           (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_integer())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateFPToSI(val->get_llvm(), destTy->get_llvm_type()),
				                      destTy, false);
			} else if (destTy->is_unsigned() ||
			           (destTy->is_native_type() && destTy->as_native_type()->get_subtype()->is_unsigned())) {
				loadRef();
				return ir::Value::get(ctx->irCtx->builder.CreateFPToUI(val->get_llvm(), destTy->get_llvm_type()),
				                      destTy, false);
			} else {
				ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " +
				               ctx->color(destTy->to_string()) + " is not supported",
				           fileRange);
			}
		} else if (valType->is_choice()) {
			if ((destTy->is_underlying_type_integer() || destTy->is_underlying_type_unsigned()) &&
			    valType->as_choice()->has_provided_type() &&
			    (destTy->is_underlying_type_integer() ==
			     valType->as_choice()->get_provided_type()->is_underlying_type_integer())) {
				if (destTy->is_underlying_type_integer()) {
					auto desIntTy  = destTy->get_underlying_integer_type();
					auto provIntTy = valType->as_choice()->get_provided_type()->get_underlying_integer_type();
					if (desIntTy->is_same(provIntTy)) {
						loadRef();
						return ir::Value::get(val->get_llvm(), destTy, false);
					} else {
						ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) +
						               " is " + ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
						               ", but the type for conversion is " + ctx->color(destTy->to_string()),
						           fileRange);
					}
				} else {
					auto desIntTy  = destTy->get_underlying_unsigned_type();
					auto provIntTy = valType->as_choice()->get_provided_type()->get_underlying_unsigned_type();
					if (desIntTy->is_same(provIntTy)) {
						loadRef();
						return ir::Value::get(val->get_llvm(), destTy, false);
					} else {
						ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) +
						               " is " + ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
						               ", but the type for conversion is " + ctx->color(destTy->to_string()),
						           fileRange);
					}
				}
			} else {
				ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) + " is " +
				               ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
				               ", but the target type for conversion is " + ctx->color(destTy->to_string()),
				           fileRange);
			}
		} else if (valType->is_expanded() && valType->as_expanded()->has_to_convertor(destTy)) {
			auto convFn = valType->as_expanded()->get_to_convertor(destTy);
			if (val->get_ir_type()->is_ref()) {
				val->load_ghost_ref(ctx->irCtx->builder);
			} else if (not val->is_ghost_ref()) {
				ctx->Error(
				    "This expression is a value and hence cannot call the convertor on this expression. Expected a reference, a local or global value or something similar. Consider saving this value in a local value first and then the convertor on it",
				    source->fileRange);
			}
			return convFn->call(ctx->irCtx, {val->get_llvm()}, None, ctx->mod);
		} else if (valType->has_default_implementations()) {
			auto        defImps = valType->get_default_implementations();
			ir::Method* convFn  = nullptr;
			for (auto imp : defImps) {
				if (imp->has_to_convertor(destTy)) {
					convFn = imp->get_to_convertor(destTy);
					break;
				}
			}
			if (not convFn) {
				ctx->Error("Type " + ctx->color(valType->to_string()) +
				               " does not have any convertor that converts to the type " +
				               ctx->color(destTy->to_string()),
				           fileRange);
			}
			if (val->get_ir_type()->is_ref()) {
				val->load_ghost_ref(ctx->irCtx->builder);
			} else if (not val->is_ghost_ref()) {
				ctx->Error(
				    "This expression is a value and hence cannot call the convertor on this expression. Expected a reference, a local or global value or something similar. Consider saving this value in a local value first and then the convertor on it",
				    source->fileRange);
			}
			return convFn->call(ctx->irCtx, {val->get_llvm()}, None, ctx->mod);
		} else {
			ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
			               " is not supported",
			           fileRange);
		}
	}
	std::unreachable();
}

} // namespace qat::ast
