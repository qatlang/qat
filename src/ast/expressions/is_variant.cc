#include "./is_variant.hpp"
#include "../../IR/types/error.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/result.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

ir::Value* IsVariant::emit(EmitCtx* ctx) {
	auto val   = expression->emit(ctx);
	auto valTy = val->get_ir_type();
	if (kind == IsVariantKind::VARIABILITY) {
		if (not val->is_ref() && not val->is_ghost_ref()) {
			ctx->Error("The expression provided here is a value, and hence will always have variability. " +
			               ctx->color("'is:var") +
			               " is only useful for references or reference-like expressions like local or global values",
			           fileRange);
		}
		return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                                                   ((valTy->is_ref() && valTy->as_ref()->has_variability()) ||
		                                                    (val->is_ghost_ref() && val->is_variable()))
		                                                       ? 1u
		                                                       : 0u),
		                            ir::UnsignedType::create_bool(ctx->irCtx));
	} else {
		bool isRef = false;
		if (valTy->is_ref()) {
			isRef = true;
			valTy = valTy->as_ref()->get_subtype();
		}
		auto boolTy = ir::UnsignedType::create_bool(ctx->irCtx);
		switch (kind) {
			case IsVariantKind::BOOL_TRUE:
			case IsVariantKind::BOOL_FALSE: {
				if (not valTy->is_bool()) {
					ctx->Error("Expected an expression of type " + ctx->color("bool") +
					               " here, but got an expression of type " + ctx->color(valTy->to_string()) +
					               " instead",
					           fileRange);
				}
				val->load_ghost_ref(ctx->irCtx->builder);
				if (isRef) {
					val = ir::Value::get(
					    ctx->irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->irCtx->llctx), val->get_llvm()),
					    boolTy, true);
				}
				return ir::Value::get(kind == IsVariantKind::BOOL_TRUE ? val->get_llvm()
				                                                       : ctx->irCtx->builder.CreateNot(val->get_llvm()),
				                      boolTy, true);
			}
			case IsVariantKind::MAYBE_VALUE: {
				if (not valTy->is_maybe()) {
					ctx->Error("Expected an expression of a " + ctx->color("maybe") +
					               " type, but got an expression of type " + ctx->color(valTy->to_string()) +
					               " instead",
					           fileRange);
				}
				if (isRef) {
					val->load_ghost_ref(ctx->irCtx->builder);
				}
				llvm::Value* cand = nullptr;
				if (isRef || val->is_ghost_ref()) {
					cand = ctx->irCtx->builder.CreateLoad(
					    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
					    ctx->irCtx->builder.CreateStructGEP(valTy->get_llvm_type(), val->get_llvm(), 0u));
				} else {
					cand = ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u});
				}
				return ir::Value::get(cand, boolTy, true);
			}
			case IsVariantKind::RESULT_OK:
			case IsVariantKind::RESULT_ERROR: {
				if (not valTy->is_result()) {
					ctx->Error("Expected an expression of a " + ctx->color("result") +
					               " type, but got an expression of type " + ctx->color(valTy->to_string()) +
					               " instead",
					           fileRange);
				}
				if (isRef) {
					val->load_ghost_ref(ctx->irCtx->builder);
				}
				llvm::Value* cand = nullptr;
				if (isRef || val->is_ghost_ref()) {
					cand = ctx->irCtx->builder.CreateLoad(
					    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
					    ctx->irCtx->builder.CreateStructGEP(valTy->get_llvm_type(), val->get_llvm(), 0u));
				} else {
					cand = ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u});
				}
				return ir::Value::get(kind == IsVariantKind::RESULT_OK ? cand : ctx->irCtx->builder.CreateNot(cand),
				                      boolTy, true);
			}
			case IsVariantKind::POINTER_NULL: {
				if (not valTy->is_ptr()) {
					ctx->Error("Expected an expression of a pointer type, but got an expression of type " +
					               ctx->color(valTy->to_string()) + " instead",
					           fileRange);
				}
				if (isRef) {
					val->load_ghost_ref(ctx->irCtx->builder);
				}
				llvm::Value* cand      = nullptr;
				const auto   boolTy    = ir::UnsignedType::create_bool(ctx->irCtx);
				auto         ptrTy     = valTy->as_ptr();
				auto         llvmPtrTy = llvm::PointerType::get(ctx->irCtx->llctx, ptrTy->get_address_space());
				if (ptrTy->is_multi()) {
					if (isRef || val->is_ghost_ref()) {
						cand = ctx->irCtx->builder.CreateLoad(
						    llvmPtrTy,
						    ctx->irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), val->get_llvm(), 0u));
					} else {
						cand = ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u});
					}
				} else {
					cand = val->get_llvm();
					if (isRef || val->is_ghost_ref()) {
						cand = ctx->irCtx->builder.CreateLoad(llvmPtrTy, cand);
					}
				}
				return ir::Value::get(
				    ctx->irCtx->builder.CreateICmpEQ(
				        ctx->irCtx->builder.CreatePtrDiff(llvm::Type::getInt8Ty(ctx->irCtx->llctx), cand,
				                                          llvm::ConstantPointerNull::get(llvmPtrTy)),
				        llvm::ConstantInt::get(ir::NativeType::get_ptrdiff_unsigned(ctx->irCtx)->get_llvm_type(), 0u)),
				    boolTy, true);
			}
			case IsVariantKind::NONE: {
				if (valTy->is_maybe()) {
					if (isRef) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					llvm::Value* cand = nullptr;
					if (isRef || val->is_ghost_ref()) {
						cand = ctx->irCtx->builder.CreateLoad(
						    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
						    ctx->irCtx->builder.CreateStructGEP(valTy->get_llvm_type(), val->get_llvm(), 0u));
					} else {
						cand = ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u});
					}
					return ir::Value::get(cand, boolTy, true);
				} else if (valTy->is_mix()) {
					auto mxTy = valTy->as_mix();
					if (not mxTy->has_none_variant()) {
						ctx->Error(
						    "The mix type " + ctx->color(mxTy->to_string()) +
						        " does not have the none variant, so this condition is technically always false, and unnecessary",
						    fileRange);
					}
					if (isRef) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					llvm::Value* cand = nullptr;
					if (isRef || val->is_ghost_ref()) {
						cand = ctx->irCtx->builder.CreateLoad(
						    llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u), val->get_llvm());
					} else {
						cand = ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u});
					}
					return ir::Value::get(
					    ctx->irCtx->builder.CreateICmpEQ(
					        cand, llvm::ConstantInt::get(
					                  llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u), 0u)),
					    boolTy, true);
				} else if (valTy->is_choice()) {
					auto chTy = valTy->as_choice();
					if (not chTy->has_none_variant()) {
						ctx->Error(
						    "The choice type " + ctx->color(chTy->to_string()) +
						        " does not have the none variant, so this condition is technically always false, and unnecessary",
						    fileRange);
					}
					val->load_ghost_ref(ctx->irCtx->builder);
					auto cand = val->get_llvm();
					if (isRef) {
						cand = ctx->irCtx->builder.CreateLoad(chTy->get_llvm_type(), cand);
					}
					return ir::Value::get(
					    ctx->irCtx->builder.CreateICmpEQ(
					        cand, llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(chTy->get_llvm_type()), 0u)),
					    boolTy, true);
				} else if (valTy->is_flag()) {
					val->load_ghost_ref(ctx->irCtx->builder);
					auto cand = val->get_llvm();
					if (isRef) {
						cand = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), cand);
					}
					return ir::Value::get(
					    ctx->irCtx->builder.CreateICmpEQ(cand, llvm::ConstantInt::get(valTy->get_llvm_type(), 0u)),
					    boolTy, true);
				} else if (valTy->is_error()) {
					auto errTy = valTy->as_error();
					if (not errTy->has_simple_move()) {
						ctx->Error("Found an expression of the error type " + ctx->color(errTy->to_string()) +
						               " here, but it does not have the " + ctx->color("error::none") +
						               " variant available, as its underlying type " +
						               ctx->color(errTy->get_subtype()->to_string()) +
						               " does not have simple-move."
						               " So this condition is technically always false, and unnecessary",
						           fileRange);
					}
					val->load_ghost_ref(ctx->irCtx->builder);
					auto cand = val->get_llvm();
					if (isRef) {
						cand = ctx->irCtx->builder.CreateLoad(errTy->get_llvm_type(), cand);
					}
					if (llvm::isa<llvm::PointerType>(errTy->get_llvm_type())) {
						cand = ctx->irCtx->builder.CreatePtrDiff(
						    llvm::Type::getInt8Ty(ctx->irCtx->llctx), cand,
						    llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(errTy->get_llvm_type())));
						return ir::Value::get(
						    ctx->irCtx->builder.CreateICmpEQ(
						        cand, llvm::ConstantInt::get(
						                  ir::NativeType::get_ptrdiff_unsigned(ctx->irCtx)->get_llvm_type(), 0u)),
						    boolTy, true);
					} else if (llvm::isa<llvm::IntegerType>(errTy->get_llvm_type())) {
						return ir::Value::get(
						    ctx->irCtx->builder.CreateICmpEQ(cand, llvm::ConstantInt::get(cand->getType(), 0u)), boolTy,
						    true);
					} else {
						auto intTy = llvm::Type::getIntNTy(
						    ctx->irCtx->llctx, (uint)ctx->irCtx->dataLayout.getTypeSizeInBits(errTy->get_llvm_type()));
						cand = ctx->irCtx->builder.CreateBitCast(cand, intTy);
						return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(cand, llvm::ConstantInt::get(intTy, 0u)),
						                      boolTy, true);
					}
				} else {
					ctx->Error("The type of the expression found here is " + ctx->color(valTy->to_string()) +
					               ", which does not support the " + ctx->color("none") + " variant. The " +
					               ctx->color("none") + " variant is available only for " + ctx->color("maybe") + ", " +
					               ctx->color("mix") + ", " + ctx->color("flag") + " and " + ctx->color("error") +
					               " types",
					           fileRange);
				}
			}
			case IsVariantKind::VARIANT_NAME: {
				llvm::Value* cand = nullptr;
				if (valTy->is_choice()) {
					auto chTy = valTy->as_choice();
					if (not chTy->has_field(name->value)) {
						ctx->Error("The choice type " + ctx->color(chTy->to_string()) +
						               " does not have a variant named " + ctx->color(name->value),
						           name->range);
					}
					val->load_ghost_ref(ctx->irCtx->builder);
					cand = val->get_llvm();
					if (isRef) {
						cand = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), cand);
					}
					return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(cand, chTy->get_value_for(name->value)),
					                      boolTy, true);
				} else if (valTy->is_mix()) {
					auto mxTy = valTy->as_mix();
					if (not mxTy->has_variant_with_name(name->value).first) {
						ctx->Error("The mix type " + ctx->color(mxTy->to_string()) + " does not have a variant named " +
						               ctx->color(name->value),
						           name->range);
					}
					if (isRef) {
						val->load_ghost_ref(ctx->irCtx->builder);
					}
					llvm::Value* cand = val->get_llvm();
					if (isRef || val->is_ghost_ref()) {
						cand = ctx->irCtx->builder.CreateLoad(
						    llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u),
						    ctx->irCtx->builder.CreateStructGEP(mxTy->get_llvm_type(), cand, 0u));
					}
					return ir::Value::get(
					    ctx->irCtx->builder.CreateICmpEQ(
					        cand, llvm::ConstantInt::get(
					                  llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u),
					                  mxTy->get_index_of(name->value))),
					    boolTy, true);
				} else if (valTy->is_flag()) {
					ctx->Error(
					    "The expression is of the flag type " + ctx->color(valTy->to_string()) +
					        ". Cannot check variants for flag types like this. Use " +
					        ctx->color("'is:{ " + name->value + " }") +
					        " instead if you want to check for this variant for a flag type."
					        " Remember that such an expression checks for the entire value to be just the " +
					        ctx->color(name->value) +
					        " variant, and not partially, as in not check if this is one of the variants of the flag value",
					    fileRange);
				} else {
					ctx->Error("Trying to check if the expression is the variant named " + ctx->color(name->value) +
					               ", but the underlying type of the expression is " + ctx->color(valTy->to_string()) +
					               ", which is not a choice or mix type",
					           fileRange);
				}
				std::unreachable();
			}
			case IsVariantKind::VARIABILITY: {
				std::unreachable();
			}
		}
	}
}

} // namespace qat::ast
