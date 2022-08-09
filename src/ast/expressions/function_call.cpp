#include "./function_call.hpp"

namespace qat::ast {

IR::Value *FunctionCall::emit(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if (mod->hasFunction(name)) {
    SHOW("Getting function")
    auto *fun = mod->getFunction(
        name, utils::RequesterInfo(None, None, fileRange.file.string(), None));
    SHOW("Got function. Generating arguments")
    Vec<llvm::Value *> argValues;
    for (auto *arg : arguments) {
      argValues.push_back(arg->emit(ctx)->getLLVM());
    }
    SHOW("Argument values generated")
    return fun->call(ctx, argValues, mod);
  } else {
    ctx->Error("Function " + name + " not found", fileRange);
  }
}

nuo::Json FunctionCall::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    args.emplace_back(arg->toJson());
  }
  return nuo::Json()
      ._("nodeType", "functionCall")
      ._("function", name)
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast