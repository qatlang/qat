#include "./cast.hpp"

namespace qat::ast {

ir::Value* Cast::emit(EmitCtx* ctx) {
	auto inst = instance->emit(ctx);
	auto srcTy =
		inst->get_ir_type()->is_reference() ? inst->get_ir_type()->as_reference()->get_subtype() : inst->get_ir_type();
	auto destTy = destination->emit(ctx);
	if (ctx->irCtx->dataLayout.value().getTypeAllocSize(srcTy->get_llvm_type()) ==
		ctx->irCtx->dataLayout.value().getTypeAllocSize(destTy->get_llvm_type())) {
		if (inst->is_reference() || inst->is_ghost_reference()) {
			if (inst->is_reference()) {
				inst->load_ghost_reference(ctx->irCtx->builder);
			}
			if (srcTy->is_trivially_copyable() || srcTy->is_trivially_movable()) {
				auto instllvm = inst->get_llvm();
				inst = ir::Value::get(ctx->irCtx->builder.CreateLoad(srcTy->get_llvm_type(), inst->get_llvm()), srcTy,
									  false);
				if (!srcTy->is_trivially_copyable()) {
					if (inst->is_reference() && !inst->get_ir_type()->as_reference()->isSubtypeVariable()) {
						ctx->Error(
							"This expression is a reference without variability and hence cannot be trivially moved from",
							instance->fileRange);
					} else if (!inst->is_variable()) {
						ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
								   fileRange);
					}
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(srcTy->get_llvm_type()), instllvm);
				}
			} else {
				ctx->Error(
					"The type of the expression is " + ctx->color(srcTy->to_string()) +
						" which is not trivially copyable or trivially movable, and hence casting cannot be performed. Try using " +
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
				ctx->color(std::to_string(ctx->irCtx->dataLayout.value().getTypeAllocSize(srcTy->get_llvm_type())) +
						   " bytes") +
				" and the destination type is " + ctx->color(destTy->to_string()) + " with the allocation size of " +
				ctx->color(std::to_string(ctx->irCtx->dataLayout.value().getTypeAllocSize(destTy->get_llvm_type())) +
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
