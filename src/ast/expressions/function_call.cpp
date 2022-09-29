#include "./function_call.hpp"

namespace qat::ast {

FunctionCall::FunctionCall(Expression *_fnExpr, Vec<Expression *> _arguments,
                           utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), fnExpr(_fnExpr),
      values(std::move(_arguments)) {}

IR::Value *FunctionCall::emit(IR::Context *ctx) {
  auto *mod   = ctx->getMod();
  auto *fnVal = fnExpr->emit(ctx);
  if (fnVal && fnVal->getType()->isFunction()) {
    auto *fun = (IR::Function *)fnVal;
    if (fun->getType()->asFunction()->getArgumentTypes().size() !=
        values.size()) {
      ctx->Error("Number of arguments provided for the function call does not "
                 "match the function signature",
                 fileRange);
    }
    Vec<IR::Value *> argsEmit;
    for (auto *val : values) {
      argsEmit.push_back(val->emit(ctx));
    }
    SHOW("Argument values generated")
    auto fnArgsTy = fnVal->getType()->asFunction()->getArgumentTypes();
    for (usize i = 0; i < fnArgsTy.size(); i++) {
      if (!fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i)->getType()) ||
          (argsEmit.at(i)->getType()->isReference() &&
           !fnArgsTy.at(i)->getType()->isSame(
               argsEmit.at(i)->getType()->asReference()->getSubType()))) {
        ctx->Error("Type of this expression does not match the type of the "
                   "corresponding argument at " +
                       ctx->highlightError(std::to_string(i)) +
                       " of the function " +
                       ctx->highlightError(fun->getName()),
                   values.at(i)->fileRange);
      }
    }
    Vec<llvm::Value *> argValues;
    for (usize i = 0; i < fnArgsTy.size(); i++) {
      if (fnArgsTy.at(i)->getType()->isReference() &&
          !argsEmit.at(i)->isReference()) {
        if (!argsEmit.at(i)->isImplicitPointer()) {
          ctx->Error(
              "Cannot pass a value for the argument that expects a reference",
              values.at(i)->fileRange);
        }
      } else if (argsEmit.at(i)->isReference()) {
        argsEmit.at(i) = new IR::Value(
            ctx->builder.CreateLoad(argsEmit.at(i)->getType()->getLLVMType(),
                                    argsEmit.at(i)->getLLVM()),
            argsEmit.at(i)->getType(), argsEmit.at(i)->isVariable(),
            argsEmit.at(i)->getNature());
      } else {
        argsEmit.at(i)->loadImplicitPointer(ctx->builder);
      }
      argValues.push_back(argsEmit.at(i)->getLLVM());
    }
    return fun->call(ctx, argValues, mod);
  } else {
    ctx->Error("The expression is not callable. It has type " +
                   fnVal->getType()->toString(),
               fnExpr->fileRange);
  }
  return nullptr;
}

Json FunctionCall::toJson() const {
  Vec<JsonValue> args;
  for (auto *arg : values) {
    args.emplace_back(arg->toJson());
  }
  return Json()
      ._("nodeType", "functionCall")
      ._("function", fnExpr->toJson())
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast