#include "./default.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

ir::Value* Default::emit(EmitCtx* ctx) {
	SHOW("Emitting type for default")
	SHOW("ctx->mod is " << ctx->mod)
	auto useTy = providedType ? providedType->emit(ctx) : inferredType;
	if (providedType && is_type_inferred() && not useTy->is_same(inferredType)) {
		ctx->Error("The type provided for this expression is " + ctx->color(useTy->to_string()) +
		               ", but the type inferred from scope is " + ctx->color(inferredType->to_string()),
		           fileRange);
	}
	SHOW("Emitted type for default")
	if (useTy) {
		if (isLocalDecl()) {
			createIn = ctx->get_fn()->get_block()->new_local(irName->value, useTy, isVar, irName->range);
		} else if (canCreateIn()) {
			if (not type_check_create_in(useTy)) {
				ctx->Error(
				    "Tried to optimise the default value by creating it in-place. But the type for in-place creation is " +
				        ctx->color(createIn->get_ir_type()->to_string()) +
				        ", which does not match with the expected type, which is " + ctx->color(useTy->to_string()),
				    fileRange);
			}
		}
		if (useTy->is_integer()) {
			if (canCreateIn()) {
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(useTy->get_llvm_type(), 0u, true),
				                                createIn->get_llvm());
				return get_creation_result(ctx->irCtx, useTy, fileRange);
			}
			return ir::PrerunValue::get(llvm::ConstantInt::get(useTy->as_integer()->get_llvm_type(), 0u, true), useTy)
			    ->with_range(fileRange);
		} else if (useTy->is_unsigned()) {
			if (canCreateIn()) {
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(useTy->get_llvm_type(), 0u),
				                                createIn->get_llvm());
				return get_creation_result(ctx->irCtx, useTy, fileRange);
			}
			return ir::PrerunValue::get(llvm::ConstantInt::get(useTy->as_unsigned()->get_llvm_type(), 0u), useTy)
			    ->with_range(fileRange);

		} else if (useTy->is_mark()) {
			if (not useTy->as_mark()->is_nullable()) {
				ctx->Error("The mark type is " + ctx->color(useTy->to_string()) +
				               " which is not nullable, and hence cannot have a default value",
				           fileRange);
			}
			if (canCreateIn()) {
				ctx->irCtx->builder.CreateStore(llvm::ConstantExpr::getNullValue(useTy->get_llvm_type()),
				                                createIn->get_llvm());
				return get_creation_result(ctx->irCtx, useTy, fileRange);
			}
			return ir::PrerunValue::get(llvm::ConstantExpr::getNullValue(useTy->get_llvm_type()), useTy)
			    ->with_range(fileRange);
		} else if (useTy->is_ref()) {
			ctx->Error("Cannot get default value for a reference type", fileRange);
		} else if (useTy->is_maybe()) {
			auto* mTy   = useTy->as_maybe();
			auto* block = ctx->get_fn()->get_block();
			if (canCreateIn()) {
				ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(mTy->get_llvm_type()),
				                                createIn->get_llvm());
				return get_creation_result(ctx->irCtx, mTy, fileRange);
			}
			return ir::PrerunValue::get(llvm::Constant::getNullValue(mTy->get_llvm_type()), mTy)->with_range(fileRange);
		} else if (useTy->is_choice()) {
			auto* chTy = useTy->as_choice();
			if (not chTy->has_default()) {
				ctx->Error("Choice type " + ctx->color(useTy->to_string()) + " does not have a default value",
				           fileRange);
			}
			return ir::PrerunValue::get(chTy->get_default(), useTy)->with_range(fileRange);
		} else if (useTy->has_prerun_default_value()) {
			if (canCreateIn()) {
				ctx->irCtx->builder.CreateStore(useTy->get_prerun_default_value(ctx->irCtx)->get_llvm(),
				                                createIn->get_llvm());
				return get_creation_result(ctx->irCtx, useTy, fileRange);
			}
			return useTy->get_prerun_default_value(ctx->irCtx)->with_range(fileRange);
		} else {
			ir::Method* defFn = nullptr;
			if (useTy->is_expanded() && useTy->as_expanded()->has_default_constructor()) {
				defFn = useTy->as_expanded()->get_default_constructor();
				SHOW("Got default constructor")
			} else {
				auto& imps = useTy->get_default_implementations();
				for (auto* fn : imps) {
					if (fn->has_default_constructor()) {
						defFn = fn->get_default_constructor();
						break;
					}
				}
				SHOW("Checked default constructor in type extensions")
			}
			if (not defFn) {
				ctx->Error(
				    "Type " + ctx->color(useTy->to_string()) +
				        " does not have a default constructor or a default value. Please check logic and make necessary changes",
				    fileRange);
			}
			if (not defFn->is_accessible(ctx->get_access_info())) {
				ctx->Error("The default constructor of type " + ctx->color(useTy->to_string()) +
				               " is not accessible here",
				           fileRange);
			}
			if (not canCreateIn()) {
				createIn = ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), useTy, true,
				                                                 fileRange);
			}
			(void)defFn->call(ctx->irCtx, {createIn->get_llvm()}, None, ctx->mod);
			return get_creation_result(ctx->irCtx, useTy, fileRange);
		}
	} else {
		ctx->Error("default has no type inferred from scope, and there is no type provided like " +
		               ctx->color("default:[CustomType]"),
		           fileRange);
	}
	return nullptr;
}

Json Default::to_json() const {
	return Json()
	    ._("nodeType", "default")
	    ._("hasProvidedType", providedType != nullptr)
	    ._("providedType", (providedType ? providedType->to_json() : JsonValue()));
}

} // namespace qat::ast
