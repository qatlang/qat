#include "./not.hpp"

namespace qat::ast {

ir::Value* LogicalNot::emit(EmitCtx* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->get_ir_type();
  if (expTy->is_reference()) {
    expTy = expTy->as_reference()->get_subtype();
  }
  if (expTy->is_bool()) {
    if (expEmit->is_ghost_reference() || expEmit->is_reference()) {
      expEmit->load_ghost_reference(ctx->irCtx->builder);
      if (expEmit->is_reference()) {
        expEmit =
            ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), expEmit->get_llvm()), expTy, false);
      }
    }
    return ir::Value::get(ctx->irCtx->builder.CreateNot(expEmit->get_llvm()), expTy, false);
  } else {
    ctx->Error("Invalid expression for the not operator. The expression provided is of type " +
                   ctx->color(expTy->to_string()),
               fileRange);
  }
  return nullptr;
}

Json LogicalNot::to_json() const {
  return Json()._("nodeType", "notExpression")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
