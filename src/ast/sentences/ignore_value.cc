#include "./ignore_value.hpp"
#include "../../IR/types/void.hpp"
#include "../expression.hpp"

namespace qat::ast {

void IgnoreValue::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                      EmitCtx* ctx) {
	UPDATE_DEPS(candidate);
}

ir::Value* IgnoreValue::emit(EmitCtx* ctx) {
	(void)candidate->emit(ctx);
	return ir::Value::get(llvm::UndefValue::get(llvm::Type::getVoidTy(ctx->irCtx->llctx)),
	                      ir::VoidType::get(ctx->irCtx->llctx), false);
}

} // namespace qat::ast
