#include "./function_definition.hpp"
#include "../IR/control_flow.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionDefinition::FunctionDefinition(FunctionPrototype *_prototype,
                                       Vec<Sentence *>    _sentences,
                                       utils::FileRange   _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

IR::Value *FunctionDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::Function *)prototype->emit(ctx);
  if (fnEmit->getName() == "main" &&
      (ctx->getMod()->getFullNameWithChild("main") == "main")) {
    if (ctx->hasMain) {
      ctx->Error(ctx->highlightError("main") +
                     " function already exists. Please check the codebase",
                 prototype->fileRange);
    } else {
      auto args = fnEmit->getType()->asFunction()->getArgumentTypes();
      if (args.size() == 2) {
        if (args.at(0)->getType()->isUnsignedInteger() &&
            (args.at(0)->getType()->asUnsignedInteger()->getBitwidth() ==
             MainFirstArgBitwidth)) {
          if (args.at(1)->getType()->isPointer() &&
              ((!args.at(1)->getType()->asPointer()->isSubtypeVariable()) &&
               (args.at(1)
                    ->getType()
                    ->asPointer()
                    ->getSubType()
                    ->isCString()))) {
            ctx->hasMain = true;
          } else {
            ctx->Error(
                "The second argument of the " + ctx->highlightError("main") +
                    " function should be " + ctx->highlightError("#[cstring]"),
                prototype->arguments.at(1)->getFileRange());
          }
        } else {
          ctx->Error("The first argument of the " +
                         ctx->highlightError("main") + " function should be " +
                         ctx->highlightError("u32"),
                     prototype->arguments.at(0)->getFileRange());
        }
      } else {
        ctx->Error(
            "The " + ctx->highlightError("main") +
                " function needs 2 arguments, the first argument should be " +
                ctx->highlightError("u32") +
                ", and the second argument should be " +
                ctx->highlightError("#[cstring]"),
            prototype->fileRange);
      }
    }
  }
  ctx->fn = fnEmit;
  SHOW("Set active function: " << fnEmit->getFullName())
  auto *block = new IR::Block((IR::Function *)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  SHOW("Iteration run for function is: " << fnEmit->getName())
  for (usize i = 0; i < argIRTypes.size(); i++) {
    SHOW("Argument name is " << argIRTypes.at(i)->getName())
    SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
    auto *argVal = block->newValue(argIRTypes.at(i)->getName(),
                                   argIRTypes.at(i)->getType(),
                                   argIRTypes.at(i)->isVariable());
    SHOW("Created local value for the argument")
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i),
                             argVal->getAlloca(), false);
  }
  emitSentences(sentences, ctx);
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