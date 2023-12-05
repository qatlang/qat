#include "./function_call.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "default.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

FunctionCall::FunctionCall(Expression* _fnExpr, Vec<Expression*> _arguments, FileRange _fileRange)
    : Expression(std::move(_fileRange)), fnExpr(_fnExpr), values(std::move(_arguments)) {}

IR::Value* FunctionCall::emit(IR::Context* ctx) {
  auto* mod   = ctx->getMod();
  auto* fnVal = fnExpr->emit(ctx);
  if (fnVal && (fnVal->getType()->isFunction() ||
                (fnVal->getType()->isPointer() && fnVal->getType()->asPointer()->getSubType()->isFunction()))) {
    auto*                fnTy = fnVal->getType()->isFunction() ? fnVal->getType()->asFunction()
                                                               : fnVal->getType()->asPointer()->getSubType()->asFunction();
    Maybe<IR::Function*> fun;
    if (fnVal->getType()->isFunction()) {
      fun = (IR::Function*)fnVal;
    }
    if (fnTy->getArgumentTypes().size() != values.size()) {
      ctx->Error("Number of arguments provided for the function call does not "
                 "match the function signature",
                 fileRange);
    }
    Vec<IR::Value*> argsEmit;
    for (usize i = 0; i < values.size(); i++) {
      if (fnTy->getArgumentCount() > (u64)i) {
        auto* argTy = fnTy->getArgumentTypeAt(i)->getType();
        if (values.at(i)->hasTypeInferrance()) {
          values.at(i)->asTypeInferrable()->setInferenceType(argTy);
        }
      }
      argsEmit.push_back(values.at(i)->emit(ctx));
    }
    SHOW("Argument values generated")
    auto fnArgsTy = fnTy->getArgumentTypes();
    for (usize i = 0; i < fnArgsTy.size(); i++) {
      if (!fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i)->getType()) &&
          !fnArgsTy.at(i)->getType()->isCompatible(argsEmit.at(i)->getType()) &&
          (argsEmit.at(i)->getType()->isReference() &&
           !fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i)->getType()->asReference()->getSubType()) &&
           !fnArgsTy.at(i)->getType()->isCompatible(argsEmit.at(i)->getType()->asReference()->getSubType()))) {
        ctx->Error("Type of this expression does not match the type of the "
                   "corresponding argument at " +
                       ctx->highlightError(std::to_string(i)) + " of the function " +
                       (fun.has_value() ? ctx->highlightError(fun.value()->getName().value) : ""),
                   values.at(i)->fileRange);
      }
    }
    Vec<llvm::Value*> argValues;
    for (usize i = 0; i < fnArgsTy.size(); i++) {
      SHOW("Argument provided type at " << i << " is: " << argsEmit.at(i)->getType()->toString())
      if (fnArgsTy.at(i)->getType()->isReference() &&
          (!argsEmit.at(i)->isReference() && !argsEmit.at(i)->isImplicitPointer())) {
        ctx->Error(
            "Cannot pass a value for the argument that expects a reference. The expression provided does not reside in an address",
            values.at(i)->fileRange);
      } else if (argsEmit.at(i)->isReference()) {
        argsEmit.at(i)->loadImplicitPointer(ctx->builder);
        argsEmit.at(i) =
            new IR::Value(ctx->builder.CreateLoad(argsEmit.at(i)->getType()->asReference()->getSubType()->getLLVMType(),
                                                  argsEmit.at(i)->getLLVM()),
                          argsEmit.at(i)->getType(), argsEmit.at(i)->isVariable(), argsEmit.at(i)->getNature());
      } else {
        argsEmit.at(i)->loadImplicitPointer(ctx->builder);
      }
      argValues.push_back(argsEmit.at(i)->getLLVM());
    }
    if (fun.has_value()) {
      return fun.value()->call(ctx, argValues, None, mod);
    } else {
      fnVal->loadImplicitPointer(ctx->builder);
      return fnVal->call(ctx, argValues, None, mod);
    }
  } else {
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    ctx->Error("The expression is not callable. It has type " + fnVal->getType()->toString(), fnExpr->fileRange);
  }
  return nullptr;
}

Json FunctionCall::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : values) {
    args.emplace_back(arg->toJson());
  }
  return Json()
      ._("nodeType", "functionCall")
      ._("function", fnExpr->toJson())
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast