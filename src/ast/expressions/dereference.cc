#include "./dereference.hpp"
#include "operator.hpp"

namespace qat::ast {

ir::Value* Dereference::emit(EmitCtx* ctx) {
	auto* expEmit = exp->emit(ctx);
	auto* expTy   = expEmit->get_ir_type();
	if (expTy->is_ref()) {
		expTy = expTy->as_ref()->get_subtype();
	}
	if (expTy->is_ptr()) {
		llvm::Value* refVal = nullptr;
		auto         refTy =
		    ir::RefType::get(expTy->as_ptr()->is_subtype_variable(), expTy->as_ptr()->get_subtype(), ctx->irCtx);
		if (expEmit->is_ref()) {
			expEmit->load_ghost_ref(ctx->irCtx->builder);
			if (expTy->as_ptr()->is_multi()) {
				refVal = ctx->irCtx->builder.CreateLoad(
				    refTy->get_llvm_type(),
				    ctx->irCtx->builder.CreateStructGEP(expTy->get_llvm_type(), expEmit->get_llvm(), 0u));
			} else {
				refVal = ctx->irCtx->builder.CreateLoad(refTy->get_llvm_type(), expEmit->get_llvm());
			}
		} else if (expEmit->is_ghost_ref()) {
			if (not expTy->as_ptr()->is_multi()) {
				refVal = ctx->irCtx->builder.CreateLoad(refTy->get_llvm_type(), expEmit->get_llvm());
			} else {
				refVal = ctx->irCtx->builder.CreateLoad(
				    refTy->get_llvm_type(),
				    ctx->irCtx->builder.CreateStructGEP(expTy->get_llvm_type(), expEmit->get_llvm(), 0u));
			}
		} else if (expTy->as_ptr()->is_multi()) {
			refVal = ctx->irCtx->builder.CreateExtractValue(expEmit->get_llvm(), {0u});
		}
		// FIXME - Change implementation for each pointer ownership types
		auto res = ir::Value::get(refVal, refTy, false);
		res->set_confirmed_ref();
		return res->with_range(fileRange);
	} else {
		const auto opStr            = operator_to_string(OperatorKind::DEREFERENCE);
		auto&      imps             = expTy->get_default_implementations();
		auto       hasDerefOperator = [&]() {
            for (auto* imp : imps) {
                if (imp->has_unary_operator(opStr)) {
                    return true;
                }
            }
            return false;
		};
		auto getDerefOperator = [&]() {
			for (auto* imp : imps) {
				if (imp->has_unary_operator(opStr)) {
					return imp->get_unary_operator(opStr);
				}
			}
		};
		if ((expTy->is_expanded() && expTy->as_expanded()->has_unary_operator(opStr)) || hasDerefOperator()) {
			auto localID = expEmit->get_local_id();
			if (not expEmit->is_ref() && not expEmit->is_ghost_ref()) {
				expEmit = expEmit->make_local(ctx, None, exp->fileRange);
			} else if (expEmit->is_ref()) {
				expEmit->load_ghost_ref(ctx->irCtx->builder);
			}
			auto* uFn = expTy->is_expanded() && expTy->as_expanded()->has_unary_operator(opStr)
			                ? expTy->as_expanded()->get_unary_operator(opStr)
			                : getDerefOperator();
			return uFn->call(ctx->irCtx, {expEmit->get_llvm()}, localID, ctx->mod);
		} else {
			ctx->Error("Type " + ctx->color(expTy->to_string()) +
			               " is not a pointer type and also does not have the dereference operator " +
			               ctx->color(opStr),
			           exp->fileRange);
		}
	}
	return nullptr;
}

Json Dereference::to_json() const {
	return Json()._("nodeType", "dereference")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
