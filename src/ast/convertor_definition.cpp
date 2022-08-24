#include "./convertor_definition.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ConvertorDefinition::ConvertorDefinition(ConvertorPrototype *_prototype,
                                         Vec<Sentence *>     _sentences,
                                         utils::FileRange    _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

IR::Value *ConvertorDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active convertor function: " << fnEmit->getFullName())
  auto *block = new IR::Block((IR::Function *)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto *corePtrType =
      IR::PointerType::get(prototype->isFrom, prototype->coreType, ctx->llctx);
  auto *self = block->newValue("self", corePtrType, true);
  self->loadImplicitPointer(ctx->builder);
  if (prototype->isFrom) {
    auto *argTy  = fnEmit->getType()->asFunction()->getArgumentTypeAt(1);
    auto *argVal = block->newValue(argTy->getName(), argTy->getType(),
                                   argTy->isVariable());
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1),
                             argVal->getLLVM());
  }
  emitSentences(sentences, ctx);
  if (prototype->isFrom &&
      (block->getName() == fnEmit->getBlock()->getName())) {
    if (block->getBB()->getInstList().empty()) {
      ctx->builder.CreateRetVoid();
    } else {
      auto *lastInst = ((llvm::Instruction *)&block->getBB()->back());
      if (!llvm::isa<llvm::ReturnInst>(lastInst)) {
        ctx->builder.CreateRetVoid();
      }
    }
  }
  SHOW("Sentences emitted")
  return nullptr;
}

void ConvertorDefinition::setCoreType(IR::CoreType *coreType) {
  prototype->setCoreType(coreType);
}

nuo::Json ConvertorDefinition::toJson() const {
  Vec<nuo::JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast