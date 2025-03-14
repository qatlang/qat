#include "./error.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/result.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

void ErrorExpression::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                          EmitCtx* ctx) {
	if (errorValue) {
		UPDATE_DEPS(errorValue);
	}
	if (isPacked.has_value() && isPacked.value().second) {
		UPDATE_DEPS(isPacked.value().second);
	}
	if (providedType.has_value()) {
		UPDATE_DEPS(providedType.value().first);
		UPDATE_DEPS(providedType.value().second);
	}
}

ir::Value* ErrorExpression::emit(EmitCtx* ctx) {
	if (not ctx->has_fn()) {
		ctx->Error("Expected this expression to be inside a function", fileRange);
	}
	FnAtEnd endFn      = FnAtEnd([&]() { createIn = nullptr; });
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
		if (usableType->is_result()) {
			auto* resTy = usableType->as_result();
			if (isPacked.has_value() && (resTy->is_packed() != packValue)) {
				ctx->Error(
				    ctx->color(resTy->to_string()) + (resTy->is_packed() ? " is" : " is not") +
				        " a packed datatype, which does not match with the provided condition, which evaluates to " +
				        ctx->color(packValue ? "true" : "false"),
				    isPacked.value().first);
			}
			auto* errTy = resTy->get_error_type();
			if (not errorValue) {
				if (not errTy->is_void()) {
					ctx->Error("The inferred " + ctx->color("result") + " type for this expression is " +
					               ctx->color(resTy->to_string()) +
					               ", so expected an expression that is compatible with the error type " +
					               ctx->color(errTy->to_string()) + " to be provided",
					           fileRange);
				}
			} else if (errorValue->has_type_inferrance()) {
				errorValue->as_type_inferrable()->set_inference_type(errTy);
			}
			llvm::Value* newAlloc = nullptr;
			if (isLocalDecl()) {
				newAlloc =
				    ctx->get_fn()->get_block()->new_local(irName->value, resTy, isVar, irName->range)->get_llvm();
			}
			if (canCreateIn()) {
				if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
					ctx->Error(
					    "Tried to optimise the " + ctx->color("error") +
					        " expression by creating in-place, but the underlying type for in-place creation is " +
					        ctx->color(createIn->get_ir_type()->to_string()) + ", which is not a reference",
					    fileRange);
				}
				auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
				                                      : createIn->get_ir_type()->as_ref()->get_subtype();
				if (not type_check_create_in(usableType)) {
					ctx->Error("Tried to optimise the " + ctx->color("error") +
					               " expression by creating in-place, but the inferred type is " +
					               ctx->color(usableType->to_string()) +
					               " which does not match with the underlying type for in-place creation which is " +
					               ctx->color(expTy->to_string()),
					           fileRange);
				}
				newAlloc = createIn->get_llvm();
			} else {
				newAlloc = ir::Logic::newAlloca(ctx->get_fn(), None, resTy->get_llvm_type());
			}
			const auto shouldCreateIn = not errTy->is_void() && errorValue && errorValue->isInPlaceCreatable();
			if (shouldCreateIn) {
				errorValue->asInPlaceCreatable()->setCreateIn(
				    ir::Value::get(ctx->irCtx->builder.CreatePointerCast(
				                       ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), newAlloc, 1u),
				                       errTy->get_llvm_type()->getPointerTo(0u)),
				                   ir::RefType::get(true, errTy, ctx->irCtx), false));
			}
			auto* errVal = errorValue ? errorValue->emit(ctx) : nullptr;
			if (errTy->is_void()) {
				if (errVal && not errVal->get_ir_type()->is_void()) {
					ctx->Error("Type of this expression is " + ctx->color(errVal->get_ir_type()->to_string()) +
					               " which does not match with the error type of " + ctx->color(resTy->to_string()) +
					               ", which is " + ctx->color(errTy->to_string()),
					           errorValue->fileRange);
				}
				resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
				return ir::Value::get(newAlloc, resTy, true);
			}
			if (shouldCreateIn) {
				resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
				errorValue->asInPlaceCreatable()->unsetCreateIn();
				return get_creation_result(ctx->irCtx, resTy);
			}
			// NOTE - The following function checks for the type of the expression
			auto* finalErr = ir::Logic::handle_pass_semantics(ctx, errTy, errVal, errorValue->fileRange);
			resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
			ctx->irCtx->builder.CreateStore(
			    finalErr->get_llvm(), ctx->irCtx->builder.CreatePointerCast(
			                              ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), newAlloc, 1u),
			                              errTy->get_llvm_type()->getPointerTo(0u)));
			if (canCreateIn()) {
				return get_creation_result(ctx->irCtx, resTy);
			}
			return ir::Value::get(newAlloc, resTy, false);
		} else {
			ctx->Error("The inferred type is " + ctx->color(usableType->to_string()) +
			               " but creating an error requires a " + ctx->color("result") + " type",
			           fileRange);
		}
	} else {
		ctx->Error("No inferred type found for this expression, and no type were provided. "
		           "You can provide a type for error expression like " +
		               ctx->color("error:[validType, errorTy](expression)"),
		           fileRange);
	}
	return nullptr;
}

Json ErrorExpression::to_json() const {
	return Json()._("nodeType", "errorExpression")._("errorValue", errorValue->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
