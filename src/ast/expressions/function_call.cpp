#include "./function_call.hpp"

namespace qat::ast {

FunctionCall::FunctionCall(Expression *_fnExpr, Vec<Expression *> _arguments,
                           utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), fnExpr(_fnExpr),
      arguments(std::move(_arguments)) {}

IR::Value *FunctionCall::emit(IR::Context *ctx) {
  auto *mod    = ctx->getMod();
  auto *expVal = fnExpr->emit(ctx);
  if (expVal && expVal->getType()->isFunction()) {
    auto              *fun = (IR::Function *)expVal;
    Vec<llvm::Value *> argValues;
    for (auto *arg : arguments) {
      argValues.push_back(arg->emit(ctx)->getLLVM());
    }
    SHOW("Argument values generated")
    return fun->call(ctx, argValues, mod);
  } else {
    ctx->Error("The expression is not callable. It has type " +
                   expVal->getType()->toString(),
               fnExpr->fileRange);
  }
  return nullptr;
}

nuo::Json FunctionCall::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    args.emplace_back(arg->toJson());
  }
  return nuo::Json()
      ._("nodeType", "functionCall")
      ._("function", fnExpr->toJson())
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast