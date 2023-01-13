#include "./destructor.hpp"
#include "sentence.hpp"

namespace qat::ast {

DestructorDefinition::DestructorDefinition(FileRange _nameRange, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), nameRange(std::move(_nameRange)) {}

void DestructorDefinition::setCoreType(IR::ExpandedType* _expType) const { expType = _expType; }

void DestructorDefinition::define(IR::Context* ctx) {
  if (!expType) {
    ctx->Error("No parent type found for this member function", fileRange);
  }
  expType->createDestructor(nameRange, ctx->llctx);
  memberFn = expType->getDestructor();
}

IR::Value* DestructorDefinition::emit(IR::Context* ctx) {
  ctx->fn = memberFn;
  SHOW("Set active destructor: " << memberFn->getFullName())
  auto* block = new IR::Block(memberFn, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = memberFn->getType()->asFunction()->getArgumentTypes();
  auto* coreRefTy  = argIRTypes.at(0)->getType()->asReference();
  auto* self       = block->newValue("''", coreRefTy, true, coreRefTy->getSubType()->asCore()->getName().range);
  ctx->builder.CreateStore(memberFn->getLLVMFunction()->getArg(0u), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, memberFn, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
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