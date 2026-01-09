#include "./member_access.hpp"
#include "../../IR/types/array.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/integer.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/struct_type.hpp"
#include "../../IR/types/toggle.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../../utils/helpers.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/Casting.h>

namespace qat::ast {

ir::Value* MemberAccess::emit(EmitCtx* ctx) {
	if (isExpSelf) {
		if (ctx->get_fn()->is_method()) {
			auto* memFn = (ir::Method*)ctx->get_fn();
			if (memFn->is_static_method()) {
				ctx->Error("This is a static member function and hence cannot access members of the parent instance",
				           fileRange);
			}
		} else {
			ctx->Error(
			    "The parent function is not a member function of any type and hence cannot access members on the parent instance",
			    fileRange);
		}
	} else {
		if (instance->nodeType() == NodeType::SELF) {
			ctx->Error("Do not use this syntax for accessing members of the parent instance. Use " +
			               ctx->color("''" + name.value) + " instead",
			           fileRange);
		}
	}

	auto* inst             = instance->emit(ctx);
	auto  instAddressSpace = inst->is_ref() ? inst->get_ir_type()->as_ref()->get_address_space()
	                                        : ir::AddressSpace::get_space_for_llvm_value(ctx->irCtx, inst->get_llvm());
	auto* instType         = inst->get_ir_type();
	bool  isVar            = inst->has_variability();
	if (instType->is_ref()) {
		inst->load_ghost_ref(ctx->irCtx->builder);
		isVar    = instType->as_ref()->has_variability();
		instType = instType->as_ref()->get_subtype();
	}
	if (instType->is_array()) {
		if (name.value == "length") {
			return ir::PrerunValue::get(
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), instType->as_array()->get_length()),
			    // NOLINTNEXTLINE(readability-magic-numbers)
			    ir::IntegerType::get(64u, ctx->irCtx));
		} else {
			ctx->Error("Invalid name for member access " + ctx->color(name.value) + " for expression with type " +
			               ctx->color(instType->to_string()),
			           name.range);
		}
	} else if (instType->is_text()) {
		if (name.value == "length") {
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(1u),
				                            ir::NativeType::get_usize(ctx->irCtx));
			} else if (inst->is_value()) {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {1u}),
				                      ir::NativeType::get_usize(ctx->irCtx), false);
			} else {
				auto usizeTy = ir::NativeType::get_usize(ctx->irCtx);
				return ir::Value::get(ctx->irCtx->builder.CreateLoad(
				                          usizeTy->get_llvm_type(),
				                          ctx->irCtx->builder.CreateStructGEP(
				                              ir::TextType::get(ctx->irCtx)->get_llvm_type(), inst->get_llvm(), 1u)),
				                      usizeTy, true);
			}
		} else if (name.value == "data") {
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(0u),
				                            ir::PtrType::get(false, ir::UnsignedType::create(8u, ctx->irCtx), false,
				                                             ir::PtrOwner::of_none(), false, None, ctx->irCtx));
			} else if (inst->is_value()) {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}),
				                      ir::PtrType::get(false, ir::UnsignedType::create(8u, ctx->irCtx), false,
				                                       ir::PtrOwner::of_none(), false, None, ctx->irCtx),
				                      false);
			} else {
				SHOW("Text is an implicit pointer or a reference or pointer")
				// FIXME - Address space fix
				auto dataPtrTy = ir::PtrType::get(false, ir::UnsignedType::create(8u, ctx->irCtx), false,
				                                  ir::PtrOwner::of_none(), false, None, ctx->irCtx);
				return ir::Value::get(ctx->irCtx->builder.CreateLoad(
				                          dataPtrTy->get_llvm_type(),
				                          ctx->irCtx->builder.CreateStructGEP(
				                              ir::TextType::get(ctx->irCtx)->get_llvm_type(), inst->get_llvm(), 0u)),
				                      dataPtrTy, false);
			}
		} else {
			ctx->Error("Invalid name for member access: " + ctx->color(name.value) + " for expression of type " +
			               instType->to_string(),
			           name.range);
		}
	} else if (instType->is_future()) {
		// FIXME - ?? Also support values if possible
		if (inst->is_value()) {
			inst = inst->make_local(ctx, None, instance->fileRange);
		}
		if (name.value == "isDone") {
			return ir::Value::get(
			    ctx->irCtx->builder.CreateLoad(
			        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			        ctx->irCtx->builder.CreatePointerCast(
			            ctx->irCtx->builder.CreateInBoundsGEP(
			                llvm::Type::getInt64Ty(ctx->irCtx->llctx),
			                ctx->irCtx->builder.CreateLoad(
			                    llvm::PointerType::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
			                                           ctx->irCtx->dataLayout.getProgramAddressSpace()),
			                    ctx->irCtx->builder.CreateStructGEP(instType->as_future()->get_llvm_type(),
			                                                        inst->get_llvm(), 1u)),
			                {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
			            llvm::PointerType::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			                                   ctx->irCtx->dataLayout.getProgramAddressSpace())),
			        true),
			    ir::UnsignedType::create_bool(ctx->irCtx), false);
		} else if (name.value == "isNotDone") {
			return ir::Value::get(
			    ctx->irCtx->builder.CreateICmpEQ(
			        ctx->irCtx->builder.CreateLoad(
			            llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			            ctx->irCtx->builder.CreatePointerCast(
			                ctx->irCtx->builder.CreateInBoundsGEP(
			                    llvm::Type::getInt64Ty(ctx->irCtx->llctx),
			                    ctx->irCtx->builder.CreateLoad(
			                        llvm::PointerType::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
			                                               ctx->irCtx->dataLayout.getProgramAddressSpace()),
			                        ctx->irCtx->builder.CreateStructGEP(instType->as_future()->get_llvm_type(),
			                                                            inst->get_llvm(), 1u)),
			                    {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
			                llvm::PointerType::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			                                       ctx->irCtx->dataLayout.getProgramAddressSpace())),
			            true),
			        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
			    ir::UnsignedType::create_bool(ctx->irCtx), false);
		} else {
			ctx->Error("Invalid name " + ctx->color(name.value) + " for member access for type " +
			               ctx->color(instType->to_string()),
			           name.range);
		}
	} else if (instType->is_maybe()) {
		if (inst->is_value() && not instType->has_simple_move()) {
			inst = inst->make_local(ctx, None, instance->fileRange);
		}
		if (name.value == "hasValue") {
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(
				    llvm::cast<llvm::ConstantInt>(inst->get_llvm_constant()->getAggregateElement(0u)),
				    ir::UnsignedType::create_bool(ctx->irCtx));
			} else if (inst->is_value()) {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}),
				                      ir::UnsignedType::create_bool(ctx->irCtx), false);
			} else {
				return ir::Value::get(
				    ctx->irCtx->builder.CreateICmpEQ(
				        ctx->irCtx->builder.CreateLoad(
				            llvm::Type::getInt1Ty(ctx->irCtx->llctx),
				            ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 0u)),
				        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u)),
				    ir::UnsignedType::create_bool(ctx->irCtx), false);
			}
		} else if (name.value == "hasNoValue") {
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(llvm::ConstantFoldCompareInstruction(
				                                llvm::CmpInst::Predicate::ICMP_EQ,
				                                inst->get_llvm_constant()->getAggregateElement(0u),
				                                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
				                            ir::UnsignedType::create_bool(ctx->irCtx));
			} else if (inst->is_value()) {
				return ir::Value::get(
				    ctx->irCtx->builder.CreateNot(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u})),
				    ir::UnsignedType::create_bool(ctx->irCtx), false);
			} else {
				return ir::Value::get(
				    ctx->irCtx->builder.CreateICmpEQ(
				        ctx->irCtx->builder.CreateLoad(
				            llvm::Type::getInt1Ty(ctx->irCtx->llctx),
				            ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 0u)),
				        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
				    ir::UnsignedType::create_bool(ctx->irCtx), false);
			}
		} else {
			ctx->Error("Invalid name " + ctx->color(name.value) + " for member access of type " +
			               ctx->color(instType->to_string()),
			           fileRange);
		}
	} else if (instType->is_toggle()) {
		auto tgTy = instType->as_toggle();
		if (not tgTy->has_variant(name.value)) {
			if (tgTy->has_normal_method(name.value) || tgTy->has_variation(name.value)) {
				ctx->Error("Found a " + String(tgTy->has_normal_method(name.value) ? "normal" : "variation") +
				               " method named " + ctx->color(name.value) + " for the toggle type " +
				               ctx->color(tgTy->to_string()) +
				               ", but extracting a pointer to a normal or variation method is not allowed",
				           name.range);
			} else if (tgTy->has_static_method(name.value)) {
				ctx->Error(
				    "Found a static method named " + ctx->color(name.value) + " for the toggle type " +
				        ctx->color(tgTy->to_string()) +
				        ", but extracting a pointer to a static method is not allowed through this syntax. Use " +
				        ctx->color(tgTy->to_string() + ":" + name.value) + " instead",
				    name.range);
			} else {
				ctx->Error("The toggle type " + ctx->color(tgTy->to_string()) + " has no variant named " +
				               ctx->color(name.value),
				           name.range);
			}
		}
		auto varTy        = tgTy->get_variant_type_of(name.value);
		bool isDefaultVar = tgTy->is_default_variant(name.value);
		if (isDefaultVar) {
			if (inst->is_value()) {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}), varTy, true);
			} else {
				auto varRefTy = ir::RefType::get(isVar, varTy, instAddressSpace, ctx->irCtx);
				return ir::Value::get(
				    ctx->irCtx->builder.CreatePointerCast(
				        ctx->irCtx->builder.CreateStructGEP(tgTy->get_llvm_type(), inst->get_llvm(), 0u),
				        varRefTy->get_llvm_type()),
				    varRefTy, false);
			}
		} else {
			if (inst->is_value()) {
				return ir::Value::get(
				    ctx->irCtx->builder.CreateExtractValue(
				        ctx->irCtx->builder.CreateTruncOrBitCast(
				            inst->get_llvm(),
				            llvm::StructType::get(ctx->irCtx->llctx, {varTy->get_llvm_type()},
				                                  llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->isPacked())),
				        {0u}),
				    varTy, true);
			} else {
				auto varRefTy = ir::RefType::get(isVar, varTy, instAddressSpace, ctx->irCtx);
				return ir::Value::get(
				    ctx->irCtx->builder.CreatePointerCast(
				        ctx->irCtx->builder.CreateStructGEP(tgTy->get_llvm_type(), inst->get_llvm(), 0u),
				        varRefTy->get_llvm_type()),
				    varRefTy, false);
			}
		}
	} else if (instType->is_expanded()) {
		if (instType->is_struct() && not instType->as_struct()->has_field_with_name(name.value)) {
			ctx->Error("Struct type " + ctx->color(instType->as_struct()->to_string()) +
			               " does not have a member field named " + ctx->color(name.value) + ". Please check the logic",
			           name.range);
		}
		auto* eTy = instType->as_expanded();
		if (eTy->is_struct() && eTy->as_struct()->has_field_with_name(name.value)) {
			auto* mem = eTy->as_struct()->get_field_at(instType->as_struct()->get_index_of(name.value).value());
			mem->add_mention(name.range);
			if (isExpSelf) {
				auto* mFn = (ir::Method*)ctx->get_fn();
				if (mFn->is_constructor()) {
					if (not mFn->is_member_initted(eTy->as_struct()->get_field_index(name.value))) {
						auto mem = eTy->as_struct()->get_field_with_name(name.value);
						if (mem->defaultValue.has_value()) {
							ctx->Error(
							    "Member field " + ctx->color(name.value) + " of parent type " +
							        ctx->color(eTy->to_string()) +
							        " is not initialised yet and hence cannot be used. The field has a default value provided,"
							        " which will be used to initialise it only at the end of this constructor",
							    fileRange);
						} else {
							ctx->Error("Member field " + ctx->color(name.value) + " of parent type " +
							               ctx->color(eTy->to_string()) +
							               " has not been initialised yet and hence cannot be used",
							           fileRange);
						}
					}
				} else {
					mFn->add_used_members(mem->name.value);
				}
			}
			if (not mem->visibility.is_accessible(ctx->get_access_info())) {
				ctx->Error("Member " + ctx->color(name.value) + " of struct type " + ctx->color(eTy->get_full_name()) +
				               " is not accessible here",
				           fileRange);
			}
			if (inst->is_value() && not instType->has_simple_move()) {
				inst = inst->make_local(ctx, None, instance->fileRange);
			}
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(
				                                instType->as_struct()->get_index_of(name.value).value()),
				                            mem->type)
				    ->with_range(fileRange);
			} else if (inst->is_value()) {
				return ir::Value::get(
				           ctx->irCtx->builder.CreateExtractValue(
				               inst->get_llvm(), {(u32)instType->as_struct()->get_index_of(name.value).value()}),
				           instType->as_struct()->get_type_of_field(name.value), true)
				    ->with_range(fileRange);
			} else {
				auto llVal =
				    ctx->irCtx->builder.CreateStructGEP(instType->as_struct()->get_llvm_type(), inst->get_llvm(),
				                                        instType->as_struct()->get_index_of(name.value).value());
				auto memValTy = instType->as_struct()->get_type_of_field(name.value);
				if (memValTy->is_ref()) {
					llVal = ctx->irCtx->builder.CreateLoad(memValTy->get_llvm_type(), llVal);
				}
				auto memRefTy = ir::RefType::get(isVar, memValTy, instAddressSpace, ctx->irCtx);
				return ir::Value::get(ctx->irCtx->builder.CreatePointerCast(llVal, memRefTy->get_llvm_type()), memRefTy,
				                      false)
				    ->with_range(fileRange);
			}
		} else if (eTy->has_normal_method(name.value) || eTy->has_variation(name.value)) {
			ctx->Error("Extracting pointers to normal and variation methods are not supported", fileRange);
		} else {
			ctx->Error("Member access of " + ctx->color(name.value) + " is not supported for expression of type " +
			               ctx->color(instType->to_string()),
			           fileRange);
		}
	} else if (instType->is_ptr() && instType->as_ptr()->is_multi()) {
		if (name.value == "length") {
			if (inst->is_prerun_value()) {
				return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(1u),
				                            ir::NativeType::get_usize(ctx->irCtx));
			} else if (inst->is_value()) {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {1u}),
				                      ir::NativeType::get_usize(ctx->irCtx), false);
			} else {
				return ir::Value::get(
				    ctx->irCtx->builder.CreateLoad(
				        ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(),
				        ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 1u)),
				    ir::NativeType::get_usize(ctx->irCtx), false);
			}
		} else {
			ctx->Error("Invalid member name for pointer datatype " + ctx->color(instType->to_string()), fileRange);
		}
	} else {
		ctx->Error("Member access for expression of type " + ctx->color(instType->to_string()) + " is not supported",
		           fileRange);
	}
	return nullptr;
}

} // namespace qat::ast
