#include "./destructor.hpp"
#include "sentence.hpp"

namespace qat::ast {

void DestructorDefinition::define(MethodState& state, IR::Context* ctx) {
  state.result = IR::MemberFunction::CreateDestructor(state.parent, nameRange, fileRange, ctx);
}

IR::Value* DestructorDefinition::emit(MethodState& state, IR::Context* ctx) {
  auto  memberFn = state.result;
  auto* oldFn    = ctx->setActiveFunction(state.result);
  SHOW("Set active destructor: " << memberFn->getFullName())
  auto* block = new IR::Block(memberFn, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes  = memberFn->getType()->asFunction()->getArgumentTypes();
  auto* parentRefTy = argIRTypes.at(0)->getType()->asReference();
  auto* self        = block->newValue("''", parentRefTy, true, state.parent->getTypeRange());
  ctx->builder.CreateStore(memberFn->getLLVMFunction()->getArg(0u), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, memberFn, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  SHOW("Destructor definition complete for " << state.parent->getParentType()->toString())
  return nullptr;
}

Json DestructorDefinition::toJson() const {
  Vec<JsonValue> sntsJson;
  for (auto* snt : sentences) {
    sntsJson.push_back(snt->toJson());
  }
  return Json()._("nodeType", "destructorDefinition")._("sentences", sntsJson)._("fileRange", fileRange);
}

} // namespace qat::ast