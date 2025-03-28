#include "./dereference.hpp"

namespace qat::ast {

ir::Value* Dereference::emit(EmitCtx* ctx) {
	auto* expEmit = exp->emit(ctx);
	auto* expTy   = expEmit->get_ir_type();
	if (expTy->is_ref()) {
		expEmit->load_ghost_ref(ctx->irCtx->builder);
		expTy = expTy->as_ref()->get_subtype();
	}
	if (expTy->is_mark()) {
		if (expEmit->is_ref()) {
			expEmit->load_ghost_ref(ctx->irCtx->builder);
			if (not expTy->as_mark()->is_slice()) {
				expEmit = ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), expEmit->get_llvm()),
				                         expTy, false);
			}
		} else if (expEmit->is_ghost_ref()) {
			if (not expTy->as_mark()->is_slice()) {
				expEmit->load_ghost_ref(ctx->irCtx->builder);
			}
		} else {
			if (expTy->as_mark()->is_slice()) {
				expEmit = expEmit->make_local(ctx, None, exp->fileRange);
			}
		}
		return ir::Value::get(
		    expTy->as_mark()->is_slice()
		        ? ctx->irCtx->builder.CreateLoad(
		              llvm::PointerType::get(expTy->as_mark()->get_subtype()->get_llvm_type(),
		                                     ctx->irCtx->dataLayout.getProgramAddressSpace()),
		              ctx->irCtx->builder.CreateStructGEP(expTy->get_llvm_type(), expEmit->get_llvm(), 0u))
		        : expEmit->get_llvm(),
		    ir::RefType::get(expTy->as_mark()->is_subtype_variable(), expTy->as_mark()->get_subtype(), ctx->irCtx),
		    false);
	} else if (expTy->is_expanded()) {
		if (expTy->as_expanded()->has_unary_operator("@")) {
			auto localID = expEmit->get_local_id();
			if (not expEmit->is_ref() && not expEmit->is_ghost_ref()) {
				expEmit = expEmit->make_local(ctx, None, exp->fileRange);
			} else if (expEmit->is_ref()) {
				expEmit->load_ghost_ref(ctx->irCtx->builder);
			}
			auto* uFn = expTy->as_expanded()->get_unary_operator("@");
			return uFn->call(ctx->irCtx, {expEmit->get_llvm()}, localID, ctx->mod);
		} else {
			ctx->Error("Struct type " + ctx->color(expTy->as_struct()->get_full_name()) +
			               " does not have the dereference operator #",
			           exp->fileRange);
		}
	} else {
		ctx->Error("Expression being dereferenced is of the incorrect type", exp->fileRange);
	}
	return nullptr;
}

Json Dereference::to_json() const {
	return Json()._("nodeType", "dereference")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
