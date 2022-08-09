#include "./function_definition.hpp"
#include "../show.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

FunctionDefinition::FunctionDefinition(FunctionPrototype *_prototype,
                                       Vec<Sentence *>    _sentences,
                                       utils::FileRange   _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

IR::Value *FunctionDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::Function *)prototype->emit(ctx);
  if (fnEmit->getName() == "main") {
    ctx->hasMain = true;
  }
  ctx->fn = fnEmit;
  SHOW("Set active function: " << fnEmit->getFullName())
  auto *block = new IR::Block((IR::Function *)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  for (usize i = 0; i < argIRTypes.size(); i++) {
    SHOW("Iteration run")
    SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
    if (argIRTypes.at(i)->isVariable()) {
      SHOW("Argument is variable")
      auto *argVal = block->newValue(argIRTypes.at(i)->getName(),
                                     argIRTypes.at(i)->getType(), true);
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i),
                               argVal->getAlloca(), false);
    }
  }
  for (auto *snt : sentences) {
    (void)snt->emit(ctx);
  }
  if (fnEmit->getType()->asFunction()->getReturnType()->isVoid() &&
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

nuo::Json FunctionDefinition::toJson() const {
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