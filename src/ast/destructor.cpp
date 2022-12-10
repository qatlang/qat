#include "./destructor.hpp"
#include "sentence.hpp"

namespace qat::ast {

DestructorDefinition::DestructorDefinition(FileRange _nameRange, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), nameRange(std::move(_nameRange)) {}

void DestructorDefinition::setCoreType(IR::CoreType* _coreType) const { coreType = _coreType; }

void DestructorDefinition::define(IR::Context* ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  if (coreType->hasDestructor()) {
    ctx->Error("Core type " + ctx->highlightError(coreType->getFullName()) +
                   " already has a destructor. Please check logic and make "
                   "necessary changes",
               fileRange);
  }
  memberFn = IR::MemberFunction::CreateDestructor(coreType, nameRange, fileRange, ctx->llctx);
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
  auto* corePtrTy  = argIRTypes.at(0)->getType()->asPointer();
  auto* self       = block->newValue("''", corePtrTy, true);
  ctx->builder.CreateStore(memberFn->getLLVMFunction()->getArg(0u), self->getLLVM());
  ctx->selfVal = ctx->builder.CreateLoad(corePtrTy->getLLVMType(), self->getAlloca());
  emitSentences(sentences, ctx);
  ctx->selfVal = nullptr;
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