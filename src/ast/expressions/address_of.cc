#include "./address_of.hpp"

namespace qat::ast {

ir::Value* AddressOf::emit(EmitCtx* ctx) {
  auto inst = instance->emit(ctx);
  if (inst->is_reference() || inst->is_ghost_pointer()) {
    auto subTy = inst->is_reference() ? inst->get_ir_type()->as_reference()->get_subtype() : inst->get_ir_type();
    bool isPtrVar =
        inst->is_reference() ? inst->get_ir_type()->as_reference()->isSubtypeVariable() : inst->is_variable();
    if (inst->is_reference()) {
      inst->load_ghost_pointer(ctx->irCtx->builder);
    }
    return ir::Value::get(
        inst->get_llvm(),
        ir::PointerType::get(isPtrVar, subTy, true, ir::PointerOwner::OfAnonymous(), false, ctx->irCtx), false);
  } else {
    ctx->Error("The expression provided is of type " + ctx->color(inst->get_ir_type()->to_string()) +
                   ". It is not a reference, local or global value, so its address cannot be retrieved",
               fileRange);
  }
  return nullptr;
}

Json AddressOf::to_json() const {
  return Json()._("nodeType", "addressOf")._("instance", instance->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
