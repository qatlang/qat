#include "./member_definition.hpp"
#include "../show.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

MemberDefinition::MemberDefinition(MemberPrototype *_prototype,
                                   Vec<Sentence *>  _sentences,
                                   utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

IR::Value *MemberDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active member function: " << fnEmit->getFullName())
  auto *block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto *corePtrTy  = argIRTypes.at(0)->getType()->asPointer();
  auto *self = block->newValue("self", corePtrTy, prototype->isVariationFn);
  self->loadImplicitPointer(ctx->builder);
  for (usize i = 1; i < argIRTypes.size(); i++) {
    SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
    if (argIRTypes.at(i)->isMemberArgument()) {
      auto *memPtr = ctx->builder.CreateStructGEP(
          corePtrTy->getSubType()->getLLVMType(), self->getLLVM(),
          corePtrTy->getSubType()
              ->asCore()
              ->getIndexOf(argIRTypes.at(i)->getName())
              .value());
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr,
                               false);
    } else {
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

void MemberDefinition::setCoreType(IR::CoreType *coreType) {
  prototype->setCoreType(coreType);
}

nuo::Json MemberDefinition::toJson() const {
  Vec<nuo::JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "memberDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast