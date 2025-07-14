#include "./mix_choice_initialiser.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/toggle.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

ir::Value* MixOrChoiceInitialiser::emit(EmitCtx* ctx) {
	FnAtEnd fnObj{[&] { createIn = nullptr; }};
	// FIXME - Support heaped value
	SHOW("Mix/Choice type initialiser")
	if (not type && not is_type_inferred()) {
		ctx->Error("No type is provided for this expression, and no type could be inferred from context", fileRange);
	}
	auto* typeEmit = type ? type.emit(ctx) : inferredType;
	if (type && inferredType && not typeEmit->is_same(inferredType)) {
		ctx->Error("The type provided for initialising is " + ctx->color(typeEmit->to_string()) +
		               ", which does not match the type inferred from scope, which is " +
		               ctx->color(inferredType->to_string()),
		           type.get_range());
	}
	if (typeEmit->is_mix()) {
		auto* mixTy = typeEmit->as_mix();
		if (mixTy->is_accessible(ctx->get_access_info())) {
			auto subRes = mixTy->has_variant_with_name(subName.value);
			SHOW("Subtype check")
			if (subRes.first) {
				if (subRes.second) {
					if (not expression) {
						ctx->Error("Field " + ctx->color(subName.value) + " of mix type " +
						               ctx->color(mixTy->get_full_name()) + " expects a value to be associated with it",
						           fileRange);
					}
				} else {
					if (expression) {
						ctx->Error("Field " + ctx->color(subName.value) + " of mix type " +
						               ctx->color(mixTy->get_full_name()) + " cannot have any value associated with it",
						           fileRange);
					}
				}
				llvm::Value* exp = nullptr;
				if (subRes.second) {
					auto* typ = mixTy->get_variant_with_name(subName.value);
					if (expression->has_type_inferrance()) {
						expression->as_type_inferrable()->set_inference_type(typ);
					}
					auto* expEmit = expression->emit(ctx);
					if (typ->is_same(expEmit->get_ir_type())) {
						expEmit->load_ghost_ref(ctx->irCtx->builder);
						exp = expEmit->get_llvm();
					} else if (expEmit->is_ref() && expEmit->get_ir_type()->as_ref()->get_subtype()->is_same(typ)) {
						exp = ctx->irCtx->builder.CreateLoad(
						    expEmit->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(), expEmit->get_llvm());
					} else if (typ->is_ref() && typ->as_ref()->get_subtype()->is_same(expEmit->get_ir_type())) {
						if (expEmit->is_ghost_ref()) {
							exp = expEmit->get_llvm();
						} else {
							ctx->Error("The expected type is " + ctx->color(typ->to_string()) +
							               ", but the expression is of type " +
							               ctx->color(expEmit->get_ir_type()->to_string()),
							           expression->fileRange);
						}
					} else {
						ctx->Error("The expected type is " + ctx->color(typ->to_string()) +
						               ", but the expression is of type " +
						               ctx->color(expEmit->get_ir_type()->to_string()),
						           expression->fileRange);
					}
				} else {
					exp = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, mixTy->get_data_bitwidth()),
					                             0u);
				}
				auto* index =
				    llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, mixTy->get_tag_bitwidth()),
				                           mixTy->get_index_of(subName.value));
				if (isLocalDecl()) {
					createIn = ctx->get_fn()->get_block()->new_local(irName->value, mixTy, isVar, irName->range);
				}
				if (canCreateIn()) {
					SHOW("Is createIn")
					if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
						ctx->Error(
						    "Trying to optimise the mix type initialiser by creating in-place, but the containing type is not a reference",
						    fileRange);
					}
					auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
					                                      : createIn->get_ir_type()->as_ref()->get_subtype();
					if (not type_check_create_in(mixTy)) {
						ctx->Error(
						    "Trying to optimise the mix type initialisation by creating in-place, but the expression type is " +
						        ctx->color(mixTy->to_string()) +
						        " which does not match with the underlying type for in-place creation which is " +
						        ctx->color(expTy->to_string()),
						    fileRange);
					}
				} else {
					createIn = ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), mixTy,
					                                                 true, fileRange);
				}
				SHOW("Creating mix store")
				ctx->irCtx->builder.CreateStore(
				    index, ctx->irCtx->builder.CreateStructGEP(mixTy->get_llvm_type(), createIn->get_llvm(), 0));
				if (subRes.second) {
					ctx->irCtx->builder.CreateStore(
					    exp, ctx->irCtx->builder.CreatePointerCast(
					             ctx->irCtx->builder.CreateStructGEP(mixTy->get_llvm_type(), createIn->get_llvm(), 1),
					             llvm::PointerType::get(mixTy->get_variant_with_name(subName.value)->get_llvm_type(),
					                                    ctx->irCtx->dataLayout.getProgramAddressSpace())));
				}
				return get_creation_result(ctx->irCtx, mixTy, fileRange);
			} else {
				ctx->Error("No field named " + ctx->color(subName.value) + " is present inside mix type " +
				               ctx->color(mixTy->get_full_name()),
				           fileRange);
			}
		} else {
			ctx->Error("Mix type " + ctx->color(mixTy->get_full_name()) + " is not accessible here", fileRange);
		}
	} else if (typeEmit->is_choice()) {
		if (expression) {
			ctx->Error("An expression is provided here, but the recognised type is a choice type: " +
			               ctx->color(typeEmit->to_string()) + ". Please remove the expression or check the logic here",
			           expression->fileRange);
		}
		auto* chTy = typeEmit->as_choice();
		if (chTy->has_field(subName.value)) {
			if (isLocalDecl()) {
				createIn = ctx->get_fn()->get_block()->new_local(irName->value, chTy, isVar, irName->range);
			}
			if (canCreateIn()) {
				if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
					ctx->Error(
					    "Trying to optimise the choice type initialisation by creating in-place, but the containing type is not a reference",
					    fileRange);
				}
				auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
				                                      : createIn->get_ir_type()->as_ref()->get_subtype();
				if (not type_check_create_in(chTy)) {
					ctx->Error(
					    "Trying to optimise the choice type initialisation by creating in-place, but the expression type is " +
					        ctx->color(chTy->to_string()) +
					        " which does not match with the underlying type for in-place creation which is " +
					        ctx->color(expTy->to_string()),
					    fileRange);
				}
				ctx->irCtx->builder.CreateStore(chTy->get_value_for(subName.value), createIn->get_llvm());
				return get_creation_result(ctx->irCtx, chTy, fileRange);
			} else {
				return ir::PrerunValue::get(chTy->get_value_for(subName.value), chTy)->with_range(fileRange);
			}
		} else {
			ctx->Error("Choice type " + ctx->color(chTy->to_string()) + " does not have a variant named " +
			               ctx->color(subName.value),
			           subName.range);
		}
	} else if (typeEmit->is_toggle()) {
		auto tgTy = typeEmit->as_toggle();
		if (not tgTy->has_variant(subName.value)) {
			ctx->Error("Toggle type " + ctx->color(tgTy->to_string()) + " does not have a variant named " +
			               ctx->color(subName.value),
			           subName.range);
		}
		const bool isDefaultVar = tgTy->is_default_variant(subName.value);
		auto       varType      = tgTy->get_variant_type_of(subName.value);

		if (not expression) {
			ctx->Error("The variant " + ctx->color(subName.value) + " of the toggle type " +
			               ctx->color(typeEmit->to_string()) + " expects an expression of type " +
			               ctx->color(varType->to_string()) + ". Please change this expression to " +
			               ctx->color(type.to_string() + "::" + subName.value + "(variantExpression)"),
			           subName.range);
		}
		if (expression->has_type_inferrance()) {
			expression->as_type_inferrable()->set_inference_type(varType);
		}
		if (isLocalDecl() || not canCreateIn()) {
			createIn = ctx->get_fn()->get_block()->new_local(
			    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), tgTy,
			    irName.has_value() ? isVar : true, irName.has_value() ? irName->range : fileRange);
		}
		if (expression->isInPlaceCreatable()) {

			auto elemPtr = ctx->irCtx->builder.CreateStructGEP(tgTy->get_llvm_type(), createIn->get_llvm(), 0u);
			expression->asInPlaceCreatable()->setCreateIn(ir::Value::get(
			    isDefaultVar ? elemPtr
			                 : ctx->irCtx->builder.CreatePointerCast(
			                       elemPtr, llvm::PointerType::get(varType->get_llvm_type(),
			                                                       ctx->irCtx->dataLayout.getProgramAddressSpace())),
			    ir::RefType::get(true, varType, ctx->irCtx), true));
			(void)expression->emit(ctx);
			if (llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getNumElements() == 2) {
				ctx->irCtx->builder.CreateStore(
				    llvm::ConstantAggregateZero::get(
				        llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getElementType(1u)),
				    ctx->irCtx->builder.CreateStructGEP(tgTy->get_llvm_type(), createIn->get_llvm(), 1u));
			}
			return get_creation_result(ctx->irCtx, tgTy, fileRange);
		} else {
			auto subVal = expression->emit(ctx);
			if (not subVal->get_pass_type()->is_same(varType)) {
				ctx->Error("The variant " + ctx->color(subName.value) + " of the toggle type " +
				               ctx->color(tgTy->to_string()) + " have the type " + ctx->color(varType->to_string()) +
				               " associated with it, but found an expression of type " +
				               ctx->color(subVal->get_pass_type()->to_string()),
				           expression->fileRange);
			}
			subVal = ir::Logic::handle_pass_semantics(ctx, subVal->get_pass_type(), subVal, expression->fileRange);
			if (subVal->is_prerun_value()) {
				if (isDefaultVar) {
					Vec<llvm::Constant*> toggleElems;
					toggleElems.reserve(2);
					toggleElems.push_back(subVal->get_llvm_constant());
					if (llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getNumElements() == 2) {
						toggleElems.push_back(llvm::ConstantAggregateZero::get(
						    llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getElementType(1)));
					}
					return ir::PrerunValue::get(
					    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(tgTy->get_llvm_type()), toggleElems),
					    tgTy);
				} else {
					auto toggleSize   = (usize)ctx->irCtx->dataLayout.getTypeStoreSize(tgTy->get_llvm_type());
					auto variantSize  = (usize)ctx->irCtx->dataLayout.getTypeStoreSize(varType->get_llvm_type());
					auto variantArray = llvm::cast<llvm::ConstantArray>(llvm::ConstantFoldConstant(
					    llvm::ConstantExpr::getBitCast(
					        subVal->as_prerun()->get_llvm_constant(),
					        llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), variantSize)),
					    ctx->irCtx->dataLayout));
					Vec<llvm::Constant*> toggleArrElems(
					    toggleSize, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u));
					for (usize i = 0; i < variantSize; i++) {
						toggleArrElems[i] = variantArray->getAggregateElement(i);
					}
					return ir::PrerunValue::get(
					    llvm::ConstantFoldConstant(
					        llvm::ConstantExpr::getBitCast(
					            llvm::ConstantArray::get(
					                llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), toggleSize),
					                toggleArrElems),
					            tgTy->get_llvm_type()),
					        ctx->irCtx->dataLayout),
					    tgTy);
				}
			}
			ctx->irCtx->builder.CreateStore(subVal->get_llvm(), ctx->irCtx->builder.CreateStructGEP(
			                                                        tgTy->get_llvm_type(), createIn->get_llvm(), 0u));
			if (llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getNumElements() == 2) {
				ctx->irCtx->builder.CreateStore(
				    llvm::ConstantAggregateZero::get(
				        llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getElementType(1u)),
				    ctx->irCtx->builder.CreateStructGEP(tgTy->get_llvm_type(), createIn->get_llvm(), 1u));
			}
			return get_creation_result(ctx->irCtx, tgTy, fileRange);
		}
	} else {
		ctx->Error(ctx->color(typeEmit->to_string()) + " is not a mix, choice or toggle type", fileRange);
	}
	return nullptr;
}

Json MixOrChoiceInitialiser::to_json() const {
	return Json()
	    ._("nodeType", "mixTypeInitialiser")
	    ._("hasType", (bool)type)
	    ._("type", type.to_json_value())
	    ._("subName", subName)
	    ._("hasExpression", expression != nullptr)
	    ._("expression", expression ? expression->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
