#include "./function_prototype.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

FunctionPrototype::FunctionPrototype(
    const String &_name, Vec<Argument *> _arguments, bool _isVariadic,
    QatType *_returnType, bool _is_async,
    llvm::GlobalValue::LinkageTypes _linkageType, const String &_callingConv,
    const utils::VisibilityInfo &_visibility,
    const utils::FileRange      &_fileRange)
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
  if (ctx->mod->hasFunction(name)) {
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
  function = ctx->mod->createFunction(
      name, returnType->emit(ctx), returnType->isVariable(), isAsync, args,
      isVariadic, fileRange, visibility, linkageType, ctx->llctx);
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

} // namespace qat::ast