#include "./default.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

ir::Value* Default::emit(EmitCtx* ctx) {
	SHOW("Emitting type for default")
	SHOW("ctx->mod is " << ctx->mod)
	auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
	SHOW("Emitted type for default")
	if (theType) {
		if (theType->is_integer()) {
			if (isLocalDecl()) {
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(theType->get_llvm_type(), 0u, true),
												localValue->get_alloca());
				return nullptr;
			} else if (irName.has_value()) {
				auto* block = ctx->get_fn()->get_block();
				auto* loc	= block->new_value(irName->value, theType, isVar, irName->range);
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(theType->get_llvm_type(), 0u, true),
												loc->get_alloca());
				return loc->to_new_ir_value();
			} else {
				return ir::PrerunValue::get(llvm::ConstantInt::get(theType->as_integer()->get_llvm_type(), 0u, true),
											theType);
			}
		} else if (theType->is_unsigned_integer()) {
			if (isLocalDecl()) {
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(theType->get_llvm_type(), 0u),
												localValue->get_alloca());
				return nullptr;
			} else if (irName.has_value()) {
				auto* loc = ctx->get_fn()->get_block()->new_value(irName->value, theType, isVar, irName->range);
				ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(theType->get_llvm_type(), 0u),
												loc->get_alloca());
				return loc->to_new_ir_value();
			} else {
				return ir::PrerunValue::get(llvm::ConstantInt::get(theType->as_unsigned_integer()->get_llvm_type(), 0u),
											theType);
			}
		} else if (theType->is_mark()) {
			if (theType->as_mark()->is_nullable()) {
				return ir::PrerunValue::get(llvm::ConstantExpr::getNullValue(theType->get_llvm_type()), theType);
			} else {
				ctx->Error("The pointer type is " + ctx->color(theType->to_string()) +
							   " which is not nullable, and hence cannot have a default value",
						   fileRange);
			}
		} else if (theType->is_reference()) {
			ctx->Error("Cannot get default value for a reference type", fileRange);
		} else if (theType->is_expanded()) {
			SHOW("Type is expanded")
			auto* eTy = theType->as_expanded();
			if (eTy->has_default_constructor()) {
				auto* defFn = eTy->get_default_constructor();
				SHOW("Got default constructor")
				if (!defFn->is_accessible(ctx->get_access_info())) {
					ctx->Error("The default constructor of type " + ctx->color(eTy->to_string()) +
								   " is not accessible here",
							   fileRange);
				}
				SHOW("Getting current function block")
				auto* block = ctx->get_fn()->get_block();
				SHOW("Got current function block")
				if (isLocalDecl()) {
					(void)defFn->call(ctx->irCtx, {localValue->get_alloca()}, None, ctx->mod);
					return nullptr;
				} else {
					SHOW("Creating value for default constructor call")
					auto* loc =
						block->new_value(irName.has_value() ? irName->value : utils::unique_id(), eTy, true, fileRange);
					(void)defFn->call(ctx->irCtx, {loc->get_alloca()}, None, ctx->mod);
					return loc->to_new_ir_value();
				}
			} else {
				ctx->Error("Type " + ctx->color(eTy->get_full_name()) +
							   " does not have a default constructor. Please check logic and make necessary changes",
						   fileRange);
			}
		} else if (theType->is_maybe()) {
			auto* mTy	= theType->as_maybe();
			auto* block = ctx->get_fn()->get_block();
			if (isLocalDecl()) {
				ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(mTy->get_llvm_type()),
												localValue->get_llvm());
				return nullptr;
			} else {
				// FIXME - Change after adding checks if type can be prerun
				auto* loc = block->new_value(irName.has_value() ? irName->value : utils::unique_id(), mTy, true,
											 irName.has_value() ? irName->range : fileRange);
				ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(mTy->get_llvm_type()), loc->get_llvm());
				return loc->to_new_ir_value();
			}
		} else if (theType->is_choice()) {
			auto* chTy = theType->as_choice();
			if (chTy->has_default()) {
				return ir::PrerunValue::get(chTy->get_default(), theType);
			} else {
				ctx->Error("Choice type " + ctx->color(theType->to_string()) + " does not have a default value",
						   fileRange);
			}
		} else if (theType->has_prerun_default_value()) {
			if (isLocalDecl()) {
				ctx->irCtx->builder.CreateStore(theType->get_prerun_default_value(ctx->irCtx)->get_llvm(),
												localValue->get_llvm());
				return nullptr;
			} else if (irName.has_value()) {
				auto* loc = ctx->get_fn()->get_block()->new_value(irName->value, theType, isVar, irName->range);
				ctx->irCtx->builder.CreateStore(theType->get_prerun_default_value(ctx->irCtx)->get_llvm(),
												loc->get_llvm());
				return nullptr;
			} else {
				return theType->get_prerun_default_value(ctx->irCtx);
			}
		} else {
			ctx->Error("The type " + ctx->color(theType->to_string()) + " does not have default value", fileRange);
			// FIXME - Handle other types
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
		._("hasProvidedType", providedType.has_value())
		._("providedType", (providedType.has_value() ? providedType.value()->to_json() : JsonValue()));
}

} // namespace qat::ast
