#include "./ok.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/result.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

void OkExpression::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                       EmitCtx* ctx) {
	if (subExpr) {
		UPDATE_DEPS(subExpr);
	}
	if (isPacked.has_value() && isPacked.value().second) {
		UPDATE_DEPS(isPacked.value().second);
	}
	if (providedType) {
		UPDATE_DEPS(providedType.value().first);
		UPDATE_DEPS(providedType.value().second);
	}
}

ir::Value* OkExpression::emit(EmitCtx* ctx) {
	if (not ctx->has_fn()) {
		ctx->Error("Expected this expression to be inside a function", fileRange);
	}
	FnAtEnd fnObj{[&] { createIn = nullptr; }};
	auto    usableType = inferredType;
	bool    packValue  = false;
	if (isPacked.has_value()) {
		if (isPacked.value().second) {
			auto packExp = isPacked.value().second->emit(ctx);
			if (not packExp->get_ir_type()->is_bool()) {
				ctx->Error("The condition for packing the " + ctx->color("result") + " datatype is expected to be of " +
				               ctx->color("bool") + " type. Got an expression of type " +
				               ctx->color(packExp->get_ir_type()->to_string()),
				           isPacked.value().second->fileRange);
			}
			packValue = llvm::cast<llvm::ConstantInt>(packExp->get_llvm_constant())->getValue().getBoolValue();
		} else {
			packValue = true;
		}
	}
	if (providedType.has_value()) {
		auto provResTy = providedType.value().first->emit(ctx);
		auto provErrTy = providedType.value().second->emit(ctx);
		auto provTy    = ir::ResultType::get(provResTy, provErrTy, packValue, ctx->irCtx);
		check_inferred_type(provTy, ctx, fileRange);
		usableType = provTy;
	}
	if (usableType) {
		if (not usableType->is_result()) {
			ctx->Error("Inferred type is " + ctx->color(usableType->to_string()) + " cannot be the type of " +
			               ctx->color("ok") + " expression, as it expects a result type",
			           fileRange);
		}
		auto* resTy = usableType->as_result();
		auto* valTy = resTy->get_valid_type();
		if (isPacked.has_value() && (resTy->is_packed() != packValue)) {
			ctx->Error(ctx->color(resTy->to_string()) + (resTy->is_packed() ? " is" : " is not") +
			               " a packed datatype, which does not match with the provided condition, which evaluates to " +
			               ctx->color(packValue ? "true" : "false"),
			           isPacked.value().first);
		}
		if (not subExpr) {
			if (not valTy->is_void()) {
				ctx->Error("The inferred " + ctx->color("result") + " type for this expression is " +
				               ctx->color(resTy->to_string()) +
				               ", so expected an expression that is compatible with the valid type " +
				               ctx->color(valTy->to_string()) + " to be provided",
				           fileRange);
			}
		} else if (subExpr->has_type_inferrance()) {
			subExpr->as_type_inferrable()->set_inference_type(usableType->as_result()->get_valid_type());
		}
		if (isLocalDecl()) {
			createIn = ctx->get_fn()->get_block()->new_local(irName->value, resTy, isVar, irName->range);
		}
		if (canCreateIn()) {
			if (createIn->is_ref() || createIn->is_ghost_ref()) {
				auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
				                                      : createIn->get_ir_type()->as_ref()->get_subtype();
				if (not type_check_create_in(usableType)) {
					ctx->Error("Trying to optimise the " + ctx->color("ok") +
					               " expression by creating in-place, but the expression type is " +
					               ctx->color(usableType->to_string()) +
					               " which does not match with the underlying type for in-place creation which is " +
					               ctx->color(expTy->to_string()),
					           fileRange);
				}
			} else {
				ctx->Error("Trying to optimise the " + ctx->color("ok") +
				               " expression by creating in-place, but the underlying type for in-place creation is " +
				               ctx->color(createIn->get_ir_type()->to_string()) + ", which is not a reference",
				           fileRange);
			}
		} else {
			createIn = ir::Value::get(ir::Logic::newAlloca(ctx->get_fn(), None, usableType->get_llvm_type()),
			                          usableType, true);
		}
		const auto shouldCreateIn = not resTy->is_void() && subExpr && subExpr->isInPlaceCreatable();
		if (shouldCreateIn) {
			subExpr->asInPlaceCreatable()->setCreateIn(ir::Value::get(
			    ctx->irCtx->builder.CreatePointerCast(
			        ctx->irCtx->builder.CreateStructGEP(usableType->get_llvm_type(), createIn->get_llvm(), 1u),
			        valTy->get_llvm_type()->getPointerTo(0u)),
			    ir::RefType::get(true, valTy, ctx->irCtx), false));
		}
		auto* validVal = subExpr ? subExpr->emit(ctx) : nullptr;
		if (valTy->is_void()) {
			if (validVal && not validVal->get_ir_type()->is_void()) {
				ctx->Error("Type of this expression is " + ctx->color(validVal->get_ir_type()->to_string()) +
				               " which does not match with the value type of " + ctx->color(resTy->to_string()) +
				               ", which is " + ctx->color(valTy->to_string()),
				           subExpr->fileRange);
			}
			resTy->handle_tag_store(createIn->get_llvm(), true, ctx->irCtx);
			return get_creation_result(ctx->irCtx, resTy, fileRange);
		}
		SHOW("Sub expression emitted")
		if (shouldCreateIn) {
			// NOTE - Condition means that the sub-expression has already been created in-place
			resTy->handle_tag_store(createIn->get_llvm(), true, ctx->irCtx);
			subExpr->asInPlaceCreatable()->unsetCreateIn();
			return get_creation_result(ctx->irCtx, resTy, fileRange);
		}
		// NOTE - The following function does further type checking
		auto* finalVal = ir::Logic::handle_pass_semantics(ctx, valTy, validVal, subExpr->fileRange);
		resTy->handle_tag_store(createIn->get_llvm(), true, ctx->irCtx);
		ctx->irCtx->builder.CreateStore(finalVal->get_llvm(), ctx->irCtx->builder.CreatePointerCast(
		                                                          ctx->irCtx->builder.CreateStructGEP(
		                                                              resTy->get_llvm_type(), createIn->get_llvm(), 1u),
		                                                          valTy->get_llvm_type()->getPointerTo(0u)));
		return get_creation_result(ctx->irCtx, resTy, fileRange);
	} else {
		ctx->Error("No inferred type found for this expression, and no type were provided. "
		           "You can provide a type for ok expression like " +
		               ctx->color("ok:[validType, errorType](expression)"),
		           fileRange);
	}
	return createIn;
}

Json OkExpression::to_json() const {
	return Json()._("nodeType", "ok")._("hasSubExpression", subExpr != nullptr)._("fileRange", fileRange);
}

} // namespace qat::ast
