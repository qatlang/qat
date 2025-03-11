#include "./member_initialisation.hpp"

namespace qat::ast {

ir::Value* MemberInit::emit(EmitCtx* ctx) {
	SHOW("Checking if member fn")
	if (ctx->get_fn()->is_method()) {
		SHOW("Getting function")
		auto* memFn = (ir::Method*)ctx->get_fn();
		SHOW("Doing switch")
		switch (memFn->get_method_type()) {
			case ir::MethodType::copyConstructor:
			case ir::MethodType::moveConstructor:
			case ir::MethodType::defaultConstructor:
			case ir::MethodType::fromConvertor:
			case ir::MethodType::constructor: {
				if (not isAllowed) {
					ctx->Error(
					    "Member initialisation is not supported in this scope. It can only be present in the top level scope of the " +
					        String(memFn->get_method_type() == ir::MethodType::fromConvertor ? "convertor"
					                                                                         : "constructor"),
					    fileRange);
				}
				auto*        parentTy = memFn->get_parent_type();
				ir::Type*    memTy    = nullptr;
				Maybe<usize> memIndex;
				// FIXME - Support tuple types with named member fields?
				auto selfRef = memFn->get_first_block()->get_value("''");
				if (parentTy->is_struct()) {
					if (parentTy->as_struct()->has_field_with_name(memName.value)) {
						memTy            = parentTy->as_struct()->get_type_of_field(memName.value);
						memIndex         = parentTy->as_struct()->get_index_of(memName.value);
						auto memCheckRes = memFn->is_member_initted(memIndex.value());
						if (memCheckRes.has_value()) {
							ctx->Error("Member field " + ctx->color(memName.value) + " is already initialised at " +
							               ctx->color(memCheckRes.value().start_to_string()),
							           fileRange);
						} else {
							SHOW("Adding init member")
							memFn->add_init_member({memIndex.value(), fileRange});
						}
					} else {
						ctx->Error("Parent type " + ctx->color(parentTy->to_string()) +
						               " does not have a member field named " + ctx->color(memName.value),
						           fileRange);
					}
				} else if (parentTy->is_mix()) {
					auto mixRes = parentTy->as_mix()->has_variant_with_name(memName.value);
					if (not mixRes.first) {
						ctx->Error("No variant named " + ctx->color(memName.value) + " found in parent mix type " +
						               ctx->color(parentTy->to_string()),
						           memName.range);
					}
					for (auto ind = 0; ind < parentTy->as_mix()->get_variant_count(); ind++) {
						auto memCheckRes = memFn->is_member_initted(ind);
						if (memCheckRes.has_value()) {
							ctx->Error("The mix type instance has already been initialised at " +
							               ctx->color(memCheckRes.value().start_to_string()) +
							               ". Cannot initialise again",
							           fileRange);
						}
					}
					SHOW("Adding mix init member")
					memFn->add_init_member({parentTy->as_mix()->get_index_of(memName.value), fileRange});
					if (not mixRes.second) {
						if (isInitOfMixVariantWithoutValue) {
							ctx->irCtx->builder.CreateStore(
							    llvm::ConstantInt::get(
							        llvm::Type::getIntNTy(ctx->irCtx->llctx, parentTy->as_mix()->get_tag_bitwidth()),
							        parentTy->as_mix()->get_index_of(memName.value)),
							    ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(), selfRef->get_llvm(),
							                                        0u));
							return nullptr;
						} else {
							ctx->Error(
							    "The variant " + ctx->color(memName.value) + " of mix type " +
							        ctx->color(parentTy->to_string()) +
							        " does not have a type associated with it, and hence cannot be initialised with a value. Please use " +
							        ctx->color("'' is " + memName.value + ".") + " instead",
							    memName.range);
						}
					} else {
						memTy = parentTy->as_mix()->get_variant_with_name(memName.value);
						if (isInitOfMixVariantWithoutValue) {
							auto selfVal = memFn->get_first_block()->get_value("''");
							if (memTy->has_prerun_default_value()) {
								ctx->irCtx->builder.CreateStore(
								    memTy->get_prerun_default_value(ctx->irCtx)->get_llvm(),
								    ctx->irCtx->builder.CreatePointerCast(
								        ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(),
								                                            selfVal->get_llvm(), 1u),
								        llvm::PointerType::get(memTy->get_llvm_type(),
								                               ctx->irCtx->dataLayout.getProgramAddressSpace())));
							} else if (memTy->is_default_constructible()) {
								memTy->default_construct_value(
								    ctx->irCtx,
								    ir::Value::get(
								        ctx->irCtx->builder.CreatePointerCast(
								            ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(),
								                                                selfVal->get_llvm(), 1u),
								            llvm::PointerType::get(memTy->get_llvm_type(),
								                                   ctx->irCtx->dataLayout.getProgramAddressSpace())),
								        ir::RefType::get(true, memTy, ctx->irCtx), false),
								    memFn);
							} else {
								ctx->Error(
								    "The subtype of the variant " + ctx->color(memName.value) + " of mix type " +
								        ctx->color(parentTy->to_string()) + " is " + ctx->color(memTy->to_string()) +
								        " which does not have a default value and is also not default constructible. Initialisation of the variant"
								        " without value is not supported for this variant",
								    fileRange);
							}
							ctx->irCtx->builder.CreateStore(
							    llvm::ConstantInt::get(
							        llvm::Type::getIntNTy(ctx->irCtx->llctx, parentTy->as_mix()->get_tag_bitwidth()),
							        parentTy->as_mix()->get_index_of(memName.value)),
							    ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(), selfVal->get_llvm(),
							                                        0u));
						}
					}
				} else {
					ctx->Error("Cannot use member initialisation for type " + ctx->color(parentTy->to_string()) +
					               ". It is only allowed for core & mix types",
					           fileRange);
				}
				if (value->has_type_inferrance()) {
					value->as_type_inferrable()->set_inference_type(memTy);
				}
				ir::Value* memRef = nullptr;
				if (parentTy->is_struct()) {
					SHOW("Getting value")
					memRef = ir::Value::get(ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(),
					                                                            selfRef->get_llvm(), memIndex.value()),
					                        ir::RefType::get(true, memTy, ctx->irCtx), false);
				} else {
					SHOW("Getting value")
					memRef = ir::Value::get(
					    ctx->irCtx->builder.CreatePointerCast(
					        ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(), selfRef->get_llvm(), 1u),
					        llvm::PointerType::get(memTy->get_llvm_type(),
					                               ctx->irCtx->dataLayout.getProgramAddressSpace())),
					    ir::RefType::get(true, memTy, ctx->irCtx), false);
				}
				SHOW("Got value")
				if (value->isInPlaceCreatable()) {
					value->asInPlaceCreatable()->setCreateIn(memRef);
					(void)value->emit(ctx);
					value->asInPlaceCreatable()->unsetCreateIn();
				} else {
					auto* irVal = value->emit(ctx);
					if (memTy->is_same(irVal->get_ir_type())) {
						if (not memTy->is_ref() && irVal->is_ghost_ref()) {
							if (memTy->has_simple_copy() || memTy->has_simple_move()) {
								ctx->irCtx->builder.CreateStore(
								    ctx->irCtx->builder.CreateLoad(memTy->get_llvm_type(), irVal->get_llvm()),
								    memRef->get_llvm());
								if (not memTy->has_simple_copy()) {
									if (not irVal->is_variable()) {
										ctx->Error(
										    "This expression does not have variability and hence simple-move is not possible",
										    value->fileRange);
									}
									ctx->irCtx->builder.CreateStore(
									    llvm::Constant::getNullValue(memTy->get_llvm_type()), irVal->get_llvm());
								}
							} else {
								ctx->Error("Member field type " + ctx->color(memTy->to_string()) +
								               " does not have simple-copy and simple-move. Please use " +
								               ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
								           value->fileRange);
							}
						} else {
							ctx->irCtx->builder.CreateStore(irVal->get_llvm(), memRef->get_llvm());
						}
					} else if (irVal->is_ref() && irVal->get_ir_type()->as_ref()->get_subtype()->is_same(memTy)) {
						if (memTy->has_simple_copy() || memTy->has_simple_move()) {
							ctx->irCtx->builder.CreateStore(
							    ctx->irCtx->builder.CreateLoad(memTy->get_llvm_type(), irVal->get_llvm()),
							    memRef->get_llvm());
							if (not memTy->has_simple_copy()) {
								if (not irVal->get_ir_type()->as_ref()->has_variability()) {
									ctx->Error(
									    "This expression is a reference without variability and hence simple-move is not possible",
									    value->fileRange);
								}
								ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(memTy->get_llvm_type()),
								                                irVal->get_llvm());
							}
						} else {
							ctx->Error("Member field type " + ctx->color(memTy->to_string()) +
							               " does not have simple-copy and simple-move. Please use " +
							               ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
							           value->fileRange);
						}
					} else {
						ctx->Error("Member field type is " + ctx->color(memTy->to_string()) +
						               " but the expression is of type " +
						               ctx->color(irVal->get_ir_type()->to_string()),
						           fileRange);
					}
				}
				if (parentTy->is_mix()) {
					ctx->irCtx->builder.CreateStore(
					    llvm::ConstantInt::get(
					        llvm::Type::getIntNTy(ctx->irCtx->llctx, parentTy->as_mix()->get_tag_bitwidth()),
					        parentTy->as_mix()->get_index_of(memName.value)),
					    ctx->irCtx->builder.CreateStructGEP(parentTy->get_llvm_type(), selfRef->get_llvm(), 0u));
				}
				return nullptr;
			}
			default: {
				ctx->Error("This function is not a constructor for " +
				               ctx->color(memFn->get_parent_type()->to_string()) +
				               " and hence cannot use member initialisation",
				           fileRange);
			}
		}
		return nullptr;
	} else {
		ctx->Error("This function is not a constructor of any type and hence cannot use member initialisation",
		           fileRange);
	}
}

Json MemberInit::to_json() const {
	return Json()
	    ._("nodeType", "memberInit")
	    ._("memberName", memName)
	    ._("value", value->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
