#include "./function_prototype.hpp"
#include "../show.hpp"
#include <vector>

namespace qat::ast {

IR::Value *FunctionPrototype::emit(IR::Context *ctx) {
  IR::Function      *function;
  Vec<IR::QatType *> generatedTypes;
  if (ctx->mod->hasFunction(name)) {
    ctx->Error("A function named " + name + " exists already in this scope",
               fileRange);
  }
  SHOW("Generating types")
  i32 i = 1;
  for (auto arg : arguments) {
    auto genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
    SHOW("Type number " << i)
    i++;
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
  function = ctx->mod->createFunction(name, returnType->emit(ctx),
                                      returnType->isVariable(), isAsync, args,
                                      isVariadic, fileRange, visibility);
  SHOW("Function created!!")
  if ((linkageType == llvm::GlobalValue::ExternalLinkage) &&
      (utils::stringToCallingConv(callingConv) != 1024)) {
    // TODO - Set calling convention
    SHOW("Linkage set")
  }
  return function;
}

nuo::Json FunctionPrototype::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto arg : arguments) {
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