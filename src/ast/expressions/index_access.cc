#include "./index_access.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>

namespace qat::ast {

ir::Value* IndexAccess::emit(EmitCtx* ctx) {
	auto* inst     = instance->emit(ctx);
	auto* instType = inst->get_ir_type();
	if (instType->is_reference()) {
		instType = instType->as_reference()->get_subtype();
	}
	if (index->has_type_inferrance()) {
		if (instType->is_tuple()) {
			index->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
		} else if (instType->is_array()) {
			index->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(64u, ctx->irCtx));
		} else if (instType->is_mark() || instType->is_string_slice()) {
			index->as_type_inferrable()->set_inference_type(ir::NativeType::get_usize(ctx->irCtx));
		}
	}
	auto* ind     = index->emit(ctx);
	auto* indType = ind->get_ir_type();
	auto* zero64  = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u);
	if (instType->is_tuple()) {
		if (indType->is_unsigned_integer() && ind->is_prerun_value()) {
			if (indType->as_unsigned_integer()->get_bitwidth() > 32u) {
				ctx->Error(
				    ctx->color(indType->to_string()) +
				        " is an unsupported type of the index for accessing tuple members. The maximum allowed bitwidth is 32",
				    index->fileRange);
			}
			auto indPre = (u32)(*llvm::cast<llvm::ConstantInt>(
			                         indType->as_unsigned_integer()->get_bitwidth() < 32u
			                             ? llvm::ConstantFoldIntegerCast(ind->get_llvm_constant(),
			                                                             llvm::Type::getInt32Ty(ctx->irCtx->llctx),
			                                                             false, ctx->irCtx->dataLayout.value())
			                             : ind->get_llvm_constant())
			                         ->getValue()
			                         .getRawData());
			if (indPre >= instType->as_tuple()->getSubTypeCount()) {
				ctx->Error("The index value is " + ctx->color(std::to_string(indPre)) + " which is not less than " +
				               ctx->color(std::to_string(instType->as_tuple()->getSubTypeCount())) +
				               ", the number of members in this tuple",
				           index->fileRange);
			}
			if (inst->is_reference() || inst->is_ghost_reference()) {
				bool isMemVar = inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
				                                     : inst->is_variable();
				if (inst->is_reference()) {
					inst->load_ghost_reference(ctx->irCtx->builder);
				}
				return ir::Value::get(
				    ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), indPre),
				    ir::ReferenceType::get(isMemVar, instType->as_tuple()->getSubtypeAt(indPre), ctx->irCtx), false);
			} else if (inst->is_prerun_value()) {
				if (llvm::isa<llvm::ConstantExpr>(inst->get_llvm_constant())) {
					return ir::PrerunValue::get(
					    llvm::ConstantFoldConstant(inst->get_llvm_constant(), ctx->irCtx->dataLayout.value()),
					    instType->as_tuple()->getSubtypeAt(indPre));
				} else {
					return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(indPre),
					                            instType->as_tuple()->getSubtypeAt(indPre));
				}
			} else {
				return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {indPre}),
				                      instType->as_tuple()->getSubtypeAt(indPre), false);
			}
		} else {
			ctx->Error("Tuple members can only be accessed using prerun unsigned integers of the " + ctx->color("u32") +
			               " type",
			           index->fileRange);
		}
	} else if (instType->is_mark() || instType->is_array()) {
		ind->load_ghost_reference(ctx->irCtx->builder);
		if (ind->get_ir_type()->is_reference()) {
			indType = indType->as_reference()->get_subtype();
		}
		if (indType->is_unsigned_integer() || (indType->is_native_type() && indType->as_native_type()->is_usize())) {
			if (inst->get_ir_type()->is_reference() &&
			    (inst->get_ir_type()->as_reference()->get_subtype()->is_mark() &&
			     !inst->get_ir_type()->as_reference()->get_subtype()->as_mark()->is_slice())) {
				SHOW("Instance for member access is a Reference to: "
				     << inst->get_ir_type()->as_reference()->get_subtype()->to_string())
				inst = ir::Value::get(ctx->irCtx->builder.CreateLoad(instType->get_llvm_type(), inst->get_llvm()),
				                      instType, inst->get_ir_type()->as_reference()->isSubtypeVariable());
			}
			if (ind->get_ir_type()->is_reference()) {
				ind = ir::Value::get(ctx->irCtx->builder.CreateLoad(indType->get_llvm_type(), ind->get_llvm()), indType,
				                     ind->get_ir_type()->as_reference()->isSubtypeVariable());
			}
			if (instType->is_mark()) {
				if (!instType->as_mark()->is_slice()) {
					ctx->Error("Only slices can be indexed into", fileRange);
				}
				auto* lenExceedTrueBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
				auto* restBlock          = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block()->get_parent());
				restBlock->link_previous_block(ctx->get_fn()->get_block());
				auto ptrLen = ctx->irCtx->builder.CreateLoad(
				    ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(),
				    ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 1u));
				ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGE(ind->get_llvm(), ptrLen),
				                                 lenExceedTrueBlock->get_bb(), restBlock->get_bb());
				lenExceedTrueBlock->set_active(ctx->irCtx->builder);
				ir::Logic::panic_in_function(
				    ctx->get_fn(),
				    {ir::StringSliceType::create_value(ctx->irCtx, "The index is "), ind,
				     ir::StringSliceType::create_value(ctx->irCtx,
				                                       " which is not less than the length of the slice, which is "),
				     ir::Value::get(ptrLen, ir::NativeType::get_usize(ctx->irCtx), false)},
				    {}, index->fileRange, ctx);
				(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
				restBlock->set_active(ctx->irCtx->builder);
				Vec<llvm::Value*> idxs;
				idxs.push_back(ind->get_llvm());
				return ir::Value::get(
				    ctx->irCtx->builder.CreateInBoundsGEP(
				        instType->as_mark()->get_subtype()->get_llvm_type(),
				        ctx->irCtx->builder.CreateLoad(
				            llvm::PointerType::get(instType->as_mark()->get_subtype()->get_llvm_type(),
				                                   ctx->irCtx->dataLayout->getProgramAddressSpace()),
				            ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 0u)),
				        idxs),
				    ir::ReferenceType::get(instType->as_mark()->is_subtype_variable(),
				                           instType->as_mark()->get_subtype(), ctx->irCtx),
				    instType->as_mark()->is_subtype_variable());
			} else {
				if (inst->is_reference()) {
					inst->load_ghost_reference(ctx->irCtx->builder);
				}
				if (llvm::isa<llvm::ConstantInt>(ind->get_llvm())) {
					SHOW("Index Access : Index is a constant")
					// NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
					if (llvm::cast<llvm::ConstantInt>(ind->get_llvm())->isZero()) {
						SHOW("Returning the first element from inbounds")
						return ir::Value::get(
						    ctx->irCtx->builder.CreateInBoundsGEP(instType->get_llvm_type(), inst->get_llvm(),
						                                          {zero64, zero64}),
						    ir::ReferenceType::get(inst->is_reference()
						                               ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
						                               : inst->is_variable(),
						                           instType->as_array()->get_element_type(), ctx->irCtx),
						    false);
					}
				}
				return ir::Value::get(
				    ctx->irCtx->builder.CreateInBoundsGEP(instType->get_llvm_type(), inst->get_llvm(),
				                                          {zero64, ind->get_llvm()}),
				    ir::ReferenceType::get(inst->is_reference()
				                               ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
				                               : inst->is_variable(),
				                           instType->as_array()->get_element_type(), ctx->irCtx),
				    false);
			}
		} else {
			ctx->Error("The index is of type " + ind->get_ir_type()->to_string() + ". It should be of the " +
			               ctx->color("usize") + " type",
			           index->fileRange);
		}
	} else if (instType->is_string_slice()) {
		if (ind->is_reference() || ind->is_ghost_reference()) {
			ind->load_ghost_reference(ctx->irCtx->builder);
			if (ind->is_reference()) {
				ind = ir::Value::get(
				    ctx->irCtx->builder.CreateLoad(
				        ind->is_reference() ? ind->get_ir_type()->as_reference()->get_subtype()->get_llvm_type()
				                            : ind->get_ir_type()->get_llvm_type(),
				        ind->get_llvm()),
				    ind->is_reference() ? ind->get_ir_type()->as_reference()->get_subtype() : ind->get_ir_type(),
				    false);
				indType = ind->get_ir_type();
			}
		}
		if (!ind->get_ir_type()->is_native_type() ||
		    (ind->get_ir_type()->is_native_type() && !ind->get_ir_type()->as_native_type()->is_usize())) {
			ctx->Error(ctx->color(ind->get_ir_type()->to_string()) +
			               " is an invalid type for the index of string slice. The index should be of type " +
			               ctx->color("usize"),
			           fileRange);
		}
		if (inst->is_reference() || inst->is_ghost_reference()) {
			if (inst->is_reference()) {
				inst->load_ghost_reference(ctx->irCtx->builder);
			}
			auto  usizeTy = ir::NativeType::get_usize(ctx->irCtx);
			auto* strTy   = ir::StringSliceType::get(ctx->irCtx);
			auto* strLen  = ctx->irCtx->builder.CreateLoad(
                usizeTy->get_llvm_type(),
                ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), inst->get_llvm(), 1u));
			auto* currBlock          = ctx->get_fn()->get_block();
			auto* lenExceedTrueBlock = new ir::Block(ctx->get_fn(), currBlock);
			auto* restBlock          = new ir::Block(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGE(ind->get_llvm(), strLen),
			                                 lenExceedTrueBlock->get_bb(), restBlock->get_bb());
			lenExceedTrueBlock->set_active(ctx->irCtx->builder);
			ir::Logic::panic_in_function(
			    ctx->get_fn(),
			    {ir::StringSliceType::create_value(ctx->irCtx, "Index for string slice is "), ind,
			     ir::StringSliceType::create_value(ctx->irCtx, " which is not less than its length, which is "),
			     ir::Value::get(strLen, ir::UnsignedType::create(64u, ctx->irCtx), false)},
			    {}, fileRange, ctx);
			(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
			restBlock->set_active(ctx->irCtx->builder);
			auto* strData = ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
			                                                    inst->get_llvm(), 0u);
			SHOW("Got string data")
			SHOW("Got first element")
			return ir::Value::get(
			    ctx->irCtx->builder.CreateInBoundsGEP(
			        llvm::Type::getInt8Ty(ctx->irCtx->llctx),
			        ctx->irCtx->builder.CreateLoad(llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo(), strData),
			        Vec<llvm::Value*>({ind->get_llvm()})),
			    ir::ReferenceType::get(inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
			                                                : inst->is_variable(),
			                           ir::UnsignedType::create(8u, ctx->irCtx), ctx->irCtx),
			    false);
		} else if (inst->is_prerun_value() && ind->is_prerun_value()) {
			if (llvm::cast<llvm::ConstantInt>(
			        llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_ULT, ind->get_llvm_constant(),
			                                             inst->get_llvm_constant()->getAggregateElement(1u)))
			        ->getValue()
			        .getBoolValue()) {
				return ir::PrerunValue::get(
				    llvm::ConstantInt::get(
				        llvm::Type::getInt8Ty(ctx->irCtx->llctx),
				        llvm::cast<llvm::ConstantDataArray>(
				            inst->get_llvm_constant()->getAggregateElement(0u)->getOperand(0u))
				            ->getRawDataValues()
				            .str()
				            .at(*llvm::cast<llvm::ConstantInt>(ind->get_llvm_constant())->getValue().getRawData())),
				    ir::UnsignedType::create(8u, ctx->irCtx));
			} else {
				ctx->Error("The provided index is " +
				               ctx->color(ind->get_ir_type()->to_prerun_generic_string((ir::PrerunValue*)ind).value()) +
				               " which is not less than the length of the string slice, which is " +
				               ctx->color(ind->get_ir_type()
				                              ->to_prerun_generic_string(ir::PrerunValue::get(
				                                  inst->get_llvm_constant()->getAggregateElement(1u),
				                                  ir::UnsignedType::create(64u, ctx->irCtx)))
				                              .value()),
				           index->fileRange);
			}
		} else {
			auto* strTy              = ir::StringSliceType::get(ctx->irCtx);
			auto* strLen             = ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {1u});
			auto* currBlock          = ctx->get_fn()->get_block();
			auto* lenExceedTrueBlock = new ir::Block(ctx->get_fn(), currBlock);
			auto* restBlock          = new ir::Block(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGE(ind->get_llvm(), strLen),
			                                 lenExceedTrueBlock->get_bb(), restBlock->get_bb());
			lenExceedTrueBlock->set_active(ctx->irCtx->builder);
			ir::Logic::panic_in_function(
			    ctx->get_fn(),
			    {ir::StringSliceType::create_value(ctx->irCtx, "Index of string slice is not less than its length")},
			    {}, fileRange, ctx);
			(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
			restBlock->set_active(ctx->irCtx->builder);
			return ir::Value::get(
			    ctx->irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
			                                          ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}),
			                                          {ind->get_llvm()}),
			    ir::ReferenceType::get(inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
			                                                : inst->is_variable(),
			                           ir::UnsignedType::create(8u, ctx->irCtx), ctx->irCtx),
			    false);
		}
	} else if (instType->is_native_type() && instType->as_native_type()->is_cstring()) {
		ind->load_ghost_reference(ctx->irCtx->builder);
		inst->load_ghost_reference(ctx->irCtx->builder);
		auto* instVal = inst->get_llvm();
		if (inst->is_reference()) {
			instVal = ctx->irCtx->builder.CreateLoad(instType->get_llvm_type(), inst->get_llvm());
		}
		bool isVarExp =
		    inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable() : inst->is_variable();
		return ir::Value::get(
		    ctx->irCtx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx->irCtx->llctx), instVal, {ind->get_llvm()}),
		    ir::ReferenceType::get(isVarExp, ir::NativeType::get_char(ctx->irCtx), ctx->irCtx), false);
	} else if (instType->is_expanded()) {
		auto*         eTy      = instType->as_expanded();
		ir::Method*   opFn     = nullptr;
		ir::Value*    operand  = nullptr;
		Maybe<String> localID  = inst->get_local_id();
		bool          isVarExp = inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable()
		                                              : (inst->is_ghost_reference() ? inst->is_variable() : true);
		SHOW("Index access: isVarExp = " << isVarExp)
		if (!indType->is_reference() &&
		    ((isVarExp && eTy->has_variation_binary_operator(
		                      "[]", {ind->is_ghost_reference() ? Maybe<bool>(ind->is_variable()) : None, indType})) ||
		     eTy->has_normal_binary_operator(
		         "[]", {ind->is_ghost_reference() ? Maybe<bool>(ind->is_variable()) : None, indType}))) {
			if ((isVarExp && eTy->has_variation_binary_operator("[]", {None, indType})) ||
			    eTy->has_normal_binary_operator("[]", {None, indType})) {
				opFn = (isVarExp && eTy->has_variation_binary_operator("[]", {None, indType}))
				           ? eTy->get_variation_binary_operator("[]", {None, indType})
				           : eTy->get_normal_binary_operator("[]", {None, indType});
				if (ind->is_ghost_reference()) {
					if (indType->is_trivially_copyable() || indType->is_trivially_movable()) {
						auto indVal = ir::Value::get(
						    ctx->irCtx->builder.CreateLoad(indType->get_llvm_type(), ind->get_llvm()), indType, false);
						if (!indType->is_trivially_copyable()) {
							if (!ind->is_variable()) {
								ctx->Error(
								    "This expression does not have variability and hence cannot be trivially moved from",
								    index->fileRange);
							}
							ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(indType->get_llvm_type()),
							                                ind->get_llvm());
						}
						ind = indVal;
					} else {
						ctx->Error("This expression is of type " + ctx->color(indType->to_string()) +
						               " which cannot be trivially copied and trivially moved from",
						           index->fileRange);
					}
				}
				operand = ind;
			} else {
				opFn    = (isVarExp &&
                        eTy->has_variation_binary_operator(
                            "[]", {ind->is_ghost_reference() ? Maybe<bool>(ind->is_variable()) : None, indType}))
				              ? eTy->get_variation_binary_operator(
                                 "[]", {ind->is_ghost_reference() ? Maybe<bool>(ind->is_variable()) : None, indType})
				              : eTy->get_normal_binary_operator(
                                 "[]", {ind->is_ghost_reference() ? Maybe<bool>(ind->is_variable()) : None, indType});
				operand = ind;
			}
		} else if (indType->is_reference() &&
		           ((isVarExp &&
		             eTy->has_variation_binary_operator("[]", {None, indType->as_reference()->get_subtype()})) ||
		            eTy->has_normal_binary_operator("[]", {None, indType->as_reference()->get_subtype()}))) {
			opFn =
			    (isVarExp && eTy->has_variation_binary_operator("[]", {None, indType->as_reference()->get_subtype()}))
			        ? eTy->get_variation_binary_operator("[]", {None, indType->as_reference()->get_subtype()})
			        : eTy->get_normal_binary_operator("[]", {None, indType->as_reference()->get_subtype()});
			ind->load_ghost_reference(ctx->irCtx->builder);
			auto* indSubType = indType->as_reference()->get_subtype();
			if (indSubType->is_trivially_copyable() || indSubType->is_trivially_movable()) {
				auto indVal = ir::Value::get(
				    ctx->irCtx->builder.CreateLoad(indSubType->get_llvm_type(), ind->get_llvm()), indSubType, false);
				if (!indType->is_trivially_copyable()) {
					if (!indType->as_reference()->isSubtypeVariable()) {
						ctx->Error(
						    "This expression is a reference without variability and hence cannot be trivially moved from",
						    index->fileRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(indSubType->get_llvm_type()),
					                                ind->get_llvm());
				}
				ind = indVal;
			} else {
				ctx->Error("This expression is a reference to type " + ctx->color(indSubType->to_string()) +
				               " which cannot be trivially copied or trivially moved. Please use " +
				               ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
				           index->fileRange);
			}
			operand = ind;
		} else {
			ctx->Error("No matching [] operator found in type " + eTy->to_string() + " for index type " +
			               ctx->color(indType->to_string()),
			           index->fileRange);
		}
		if (inst->is_reference()) {
			inst->load_ghost_reference(ctx->irCtx->builder);
		}
		return opFn->call(ctx->irCtx, {inst->get_llvm(), operand->get_llvm()}, localID, ctx->mod);
	} else {
		ctx->Error("The expression of type " + instType->to_string() + " cannot be used for index access",
		           instance->fileRange);
	}
}

Json IndexAccess::to_json() const {
	return Json()
	    ._("nodeType", "memberIndexAccess")
	    ._("instance", instance->to_json())
	    ._("index", index->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
