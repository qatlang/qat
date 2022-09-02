#include "./function.hpp"
#include "../IR/control_flow.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::FunctionPrototype(
    const String &_name, Vec<Argument *> _arguments, bool _isVariadic,
    QatType *_returnType, bool _is_async,
    llvm::GlobalValue::LinkageTypes _linkageType, const String &_callingConv,
    utils::VisibilityKind _visibility, const utils::FileRange &_fileRange)
    : name(_name), isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType),
      linkageType(_linkageType), callingConv(_callingConv),
      visibility(_visibility), Node(_fileRange) {}

FunctionPrototype::FunctionPrototype(const FunctionPrototype &ref)
    : name(ref.name), isAsync(ref.isAsync), arguments(ref.arguments),
      isVariadic(ref.isVariadic), returnType(ref.returnType),
      linkageType(ref.linkageType), callingConv(ref.callingConv),
      visibility(ref.visibility), Node(ref.fileRange) {}

IR::Value *FunctionPrototype::emit(IR::Context *ctx) {
  IR::Function      *function;
  Vec<IR::QatType *> generatedTypes;
  auto              *mod = ctx->getMod();
  if (mod->hasFunction(name)) {
    ctx->Error("A function named " + name + " exists already in this scope",
               fileRange);
  }
  if (name == "main") {
    linkageType = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage;
  }
  SHOW("Generating types")
  for (auto *arg : arguments) {
    if (arg->isTypeMember()) {
      ctx->Error("Function is not a member function of a core type and cannot "
                 "use member argument syntax",
                 arg->getFileRange());
    }
    auto *genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
  }
  SHOW("Types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    args.push_back(arguments.at(i)->getType()->isVariable()
                       ? IR::Argument::CreateVariable(
                             arguments.at(i)->getName(),
                             arguments.at(i)->getType()->emit(ctx), i)
                       : IR::Argument::Create(arguments.at(i)->getName(),
                                              generatedTypes.at(i), i));
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  function = mod->createFunction(
      name, returnType->emit(ctx), returnType->isVariable(), isAsync, args,
      isVariadic, fileRange, ctx->getVisibInfo(visibility), linkageType,
      ctx->llctx);
  SHOW("Function created!!")
  // TODO - Set calling convention
  return function;
}

nuo::Json FunctionPrototype::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    auto aJson =
        nuo::Json()
            ._("name", arg->getName())
            ._("type", arg->getType() ? arg->getType()->toJson() : nuo::Json())
            ._("fileRange", arg->getFileRange());
    args.push_back(aJson);
  }
  return nuo::Json()
      ._("nodeType", "functionPrototype")
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic)
      ._("callingConvention", callingConv);
}

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