#include "./destructor.hpp"
#include "./expression.hpp"
#include "sentence.hpp"

namespace qat::ast {

void DestructorDefinition::define(MethodState& state, ir::Ctx* irCtx) {
  auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
  if (defineChecker) {
    auto checkRes = defineChecker->emit(emitCtx);
    if (checkRes->get_ir_type()->is_bool()) {
      state.defineCondition = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
      if (not state.defineCondition.value()) {
        return;
      }
    } else {
      irCtx->Error("The define condition is expected to be of type " + irCtx->color("bool") +
                       ", but instead got an expression of type " + checkRes->get_ir_type()->to_string(),
                   defineChecker->fileRange);
    }
  }
  if (metaInfo.has_value()) {
    state.metaInfo = metaInfo.value().toIR(emitCtx);
  }
  state.result = ir::Method::CreateDestructor(
      state.parent, nameRange, state.metaInfo.has_value() && state.metaInfo->get_inline(), fileRange, irCtx);
}

ir::Value* DestructorDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
  if (state.defineCondition.has_value() && (not state.defineCondition.value())) {
    return nullptr;
  }
  auto memberFn = state.result;
  SHOW("Set active destructor: " << memberFn->get_full_name())
  auto* block = new ir::Block(memberFn, nullptr);
  SHOW("Created entry block")
  block->set_active(irCtx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes  = memberFn->get_ir_type()->as_function()->get_argument_types();
  auto* parentRefTy = argIRTypes.at(0)->get_type()->as_reference();
  auto* self        = block->new_value("''", parentRefTy, true, state.parent->get_type_range());
  SHOW("Storing self")
  irCtx->builder.CreateStore(memberFn->get_llvm_function()->getArg(0u), self->get_llvm());
  SHOW("Loading self")
  self->load_ghost_reference(irCtx->builder);
  SHOW("Emitting sentences")
  emit_sentences(
      sentences,
      EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(memberFn));
  ir::function_return_handler(irCtx, memberFn, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Destructor definition complete for " << state.parent->get_parent_type()->to_string())
  return nullptr;
}

Json DestructorDefinition::to_json() const {
  Vec<JsonValue> sntsJson;
  for (auto* snt : sentences) {
    sntsJson.push_back(snt->to_json());
  }
  return Json()._("nodeType", "destructorDefinition")._("sentences", sntsJson)._("fileRange", fileRange);
}

} // namespace qat::ast
