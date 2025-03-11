#include "./cast.hpp"

namespace qat::ast {

ir::Value* Cast::emit(EmitCtx* ctx) {
	auto inst   = instance->emit(ctx);
	auto srcTy  = inst->get_ir_type()->is_ref() ? inst->get_ir_type()->as_ref()->get_subtype() : inst->get_ir_type();
	auto destTy = destination->emit(ctx);
	if (ctx->irCtx->dataLayout.getTypeAllocSize(srcTy->get_llvm_type()) ==
	    ctx->irCtx->dataLayout.getTypeAllocSize(destTy->get_llvm_type())) {
		if (inst->is_ref() || inst->is_ghost_ref()) {
			if (inst->is_ref()) {
				inst->load_ghost_ref(ctx->irCtx->builder);
			}
			if (srcTy->has_simple_copy() || srcTy->has_simple_move()) {
				auto instllvm = inst->get_llvm();
				inst = ir::Value::get(ctx->irCtx->builder.CreateLoad(srcTy->get_llvm_type(), inst->get_llvm()), srcTy,
				                      false);
				if (not srcTy->has_simple_copy()) {
					if (inst->is_ref() && not inst->get_ir_type()->as_ref()->has_variability()) {
						ctx->Error(
						    "This expression is a reference without variability and hence simple-move is not possible",
						    instance->fileRange);
					} else if (not inst->is_variable()) {
						ctx->Error("This expression does not have variability and hence simple-move is not possible",
						           fileRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(srcTy->get_llvm_type()), instllvm);
				}
			} else {
				ctx->Error(
				    "The type of the expression is " + ctx->color(srcTy->to_string()) +
				        " which does not have simple-copy and simple-move, and hence casting cannot be performed. Try using " +
				        ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
				    instance->fileRange);
			}
		}
		return ir::Value::get(ctx->irCtx->builder.CreateBitCast(inst->get_llvm(), destTy->get_llvm_type()), destTy,
		                      false)
		    ->with_range(fileRange);
	} else {
		ctx->Error(
		    "The type of the expression is " + ctx->color(srcTy->to_string()) + " with the allocation size of " +
		        ctx->color(std::to_string(ctx->irCtx->dataLayout.getTypeAllocSize(srcTy->get_llvm_type())) + " bytes") +
		        " and the destination type is " + ctx->color(destTy->to_string()) + " with the allocation size of " +
		        ctx->color(std::to_string(ctx->irCtx->dataLayout.getTypeAllocSize(destTy->get_llvm_type())) +
		                   " bytes") +
		        ". The size of these types do not match and hence casting cannot be performed",
		    fileRange);
	}
	return nullptr;
}

Json Cast::to_json() const {
	return Json()
	    ._("nodeType", "cast")
	    ._("expression", instance->to_json())
	    ._("targetType", destination->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
