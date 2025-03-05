#include "./mix_choice_initialiser.hpp"

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
						expEmit->load_ghost_reference(ctx->irCtx->builder);
						exp = expEmit->get_llvm();
					} else if (expEmit->is_reference() &&
					           expEmit->get_ir_type()->as_reference()->get_subtype()->is_same(typ)) {
						exp = ctx->irCtx->builder.CreateLoad(
						    expEmit->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
						    expEmit->get_llvm());
					} else if (typ->is_reference() &&
					           typ->as_reference()->get_subtype()->is_same(expEmit->get_ir_type())) {
						if (expEmit->is_ghost_reference()) {
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
					SHOW("Is local decl")
					if (localValue->get_ir_type()->is_same(mixTy)) {
						createIn = localValue->to_new_ir_value();
					} else {
						ctx->Error("The type of the local declaration is not compatible here", fileRange);
					}
				} else if (canCreateIn()) {
					SHOW("Is createIn")
					if (createIn->is_reference() || createIn->is_ghost_reference()) {
						auto expTy = createIn->is_ghost_reference()
						                 ? createIn->get_ir_type()
						                 : createIn->get_ir_type()->as_reference()->get_subtype();
						if (!expTy->is_same(mixTy)) {
							ctx->Error(
							    "Trying to optimise the mix type initialisation by creating in-place, but the expression type is " +
							        ctx->color(mixTy->to_string()) +
							        " which does not match with the underlying type for in-place creation which is " +
							        ctx->color(expTy->to_string()),
							    fileRange);
						}
					} else {
						ctx->Error(
						    "Trying to optimise the copy by creating in-place, but the containing type is not a reference",
						    fileRange);
					}
				} else {
					createIn = ctx->get_fn()->get_block()->new_value(
					    irName.has_value() ? irName->value : utils::uid_string(), mixTy, isVar,
					    irName.has_value() ? irName->range : fileRange);
				}
				SHOW("Creating mix store")
				ctx->irCtx->builder.CreateStore(
				    index, ctx->irCtx->builder.CreateStructGEP(mixTy->get_llvm_type(), createIn->get_llvm(), 0));
				if (subRes.second) {
					ctx->irCtx->builder.CreateStore(
					    exp, ctx->irCtx->builder.CreatePointerCast(
					             ctx->irCtx->builder.CreateStructGEP(mixTy->get_llvm_type(), createIn->get_llvm(), 1),
					             mixTy->get_variant_with_name(subName.value)->get_llvm_type()->getPointerTo()));
				}
				return createIn;
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
				ctx->irCtx->builder.CreateStore(chTy->get_value_for(subName.value), localValue->get_llvm());
				return nullptr;
			} else if (canCreateIn()) {
				if (createIn->is_reference() || createIn->is_ghost_reference()) {
					auto expTy = createIn->is_ghost_reference()
					                 ? createIn->get_ir_type()
					                 : createIn->get_ir_type()->as_reference()->get_subtype();
					if (!expTy->is_same(chTy)) {
						ctx->Error(
						    "Trying to optimise the choice type initialisation by creating in-place, but the expression type is " +
						        ctx->color(chTy->to_string()) +
						        " which does not match with the underlying type for in-place creation which is " +
						        ctx->color(expTy->to_string()),
						    fileRange);
					}
				} else {
					ctx->Error(
					    "Trying to optimise the choice type initialisation by creating in-place, but the containing type is not a reference",
					    fileRange);
				}
				ctx->irCtx->builder.CreateStore(chTy->get_value_for(subName.value), createIn->get_llvm());
			} else if (irName.has_value()) {
				auto locVal = ctx->get_fn()->get_block()->new_value(irName->value, chTy, isVar, irName->range);
				ctx->irCtx->builder.CreateStore(chTy->get_value_for(subName.value), locVal->get_llvm());
				return nullptr;
			} else {
				return ir::PrerunValue::get(chTy->get_value_for(subName.value), chTy);
			}
		} else {
			ctx->Error("Choice type " + ctx->color(chTy->to_string()) + " does not have a variant named " +
			               ctx->color(subName.value),
			           subName.range);
		}
	} else {
		ctx->Error(ctx->color(typeEmit->to_string()) + " is not a mix type or a choice type", fileRange);
	}
	return nullptr;
}

Json MixOrChoiceInitialiser::to_json() const {
	return Json()
	    ._("nodeType", "mixTypeInitialiser")
	    ._("hasType", (bool)type)
	    ._("type", (JsonValue)type)
	    ._("subName", subName)
	    ._("hasExpression", expression != nullptr)
	    ._("expression", expression ? expression->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
