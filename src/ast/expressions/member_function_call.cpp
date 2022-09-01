#include "./member_function_call.hpp"

namespace qat::ast {

IR::Value *MemberFunctionCall::emit(IR::Context *ctx) {
  SHOW("Member variable emitting")
  auto *inst     = instance->emit(ctx);
  auto *instType = inst->getType();
  bool  isVar    = inst->isVariable();
  if (instType->isReference()) {
    inst->loadImplicitPointer(ctx->builder);
    isVar    = instType->asReference()->isSubtypeVariable();
    instType = instType->asReference()->getSubType();
  }
  if (isPointerCall) {
    if (instType->isPointer()) {
      inst->loadImplicitPointer(ctx->builder);
      isVar    = instType->asPointer()->isSubtypeVariable();
      instType = instType->asPointer()->getSubType();
    } else {
      ctx->Error(
          "The expression type has to be a pointer to use > to access members",
          instance->fileRange);
    }
  } else {
    if (instType->isPointer()) {
      ctx->Error(String("The expression is of pointer type. Please use ") +
                     (variation ? ">~" : ">-") + " to access member function",
                 instance->fileRange);
    }
  }
  if (instType->isCoreType()) {
    if (!instType->asCore()->hasMemberFunction(memberName)) {
      ctx->Error(
          "Core type " + ctx->highlightError(instType->asCore()->toString()) +
              " does not have a member function named " +
              ctx->highlightError(memberName) + ". Please check the logic",
          fileRange);
    }
    auto *cTy   = instType->asCore();
    auto *memFn = cTy->getMemberFunction(memberName);
    if (variation) {
      if (!memFn->isVariationFunction()) {
        ctx->Error(
            "Member function " + ctx->highlightError(memberName) +
                " of core type " + ctx->highlightError(cTy->getFullName()) +
                " is not a variation and hence cannot be called as a variation",
            fileRange);
      }
      if (!isVar) {
        ctx->Error(
            "The expression does not have variability and hence variation "
            "member functions cannot be called",
            instance->fileRange);
      }
    } else {
      if (memFn->isVariationFunction()) {
        ctx->Error(
            "Member function " + ctx->highlightError(memberName) +
                " of core type " + ctx->highlightError(cTy->getFullName()) +
                " is a variation and hence should be called as a variation",
            fileRange);
      }
    }
    if (!memFn->isAccessible(ctx->getReqInfo())) {
      ctx->Error("Member function " + ctx->highlightError(memberName) +
                     " of core type " +
                     ctx->highlightError(cTy->getFullName()) +
                     " is not accessible here",
                 fileRange);
    }
    if (!inst->isImplicitPointer() && !inst->getType()->isReference() &&
        !inst->getType()->isPointer()) {
      inst = inst->createAlloca(ctx->builder);
    }
    //
    auto fnArgsTy = memFn->getType()->asFunction()->getArgumentTypes();
    if ((fnArgsTy.size() - 1) != arguments.size()) {
      ctx->Error(
          "Number of arguments provided for the member function call does not "
          "match the signature",
          fileRange);
    }
    Vec<IR::Value *> argsEmit;
    for (auto *val : arguments) {
      argsEmit.push_back(val->emit(ctx));
    }
    SHOW("Argument values generated")
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      if (!fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i)->getType()) &&
          (argsEmit.at(i)->getType()->isReference() &&
           !fnArgsTy.at(i)->getType()->isSame(
               argsEmit.at(i)->getType()->asReference()->getSubType()))) {
        ctx->Error("Type of this expression does not match the type of the "
                   "corresponding argument at " +
                       ctx->highlightError(std::to_string(i - 1)) +
                       " of the function " +
                       ctx->highlightError(memFn->getName()),
                   arguments.at(i)->fileRange);
      }
    }
    //
    Vec<llvm::Value *> argVals;
    argVals.push_back(inst->getLLVM());
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      if (fnArgsTy.at(i)->getType()->isReference() &&
          !argsEmit.at(i)->isReference()) {
        if (!argsEmit.at(i)->isImplicitPointer()) {
          ctx->Error(
              "Cannot pass a value for the argument that expects a reference",
              arguments.at(i)->fileRange);
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
      argVals.push_back(argsEmit.at(i)->getLLVM());
    }
    return memFn->call(ctx, argVals, ctx->getMod());
  } else {
    ctx->Error("Member function call for this type is not supported",
               fileRange);
  }
  return nullptr;
}

nuo::Json MemberFunctionCall::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    args.emplace_back(arg->toJson());
  }
  return nuo::Json()
      ._("nodeType", "memberFunctionCall")
      ._("instance", instance->toJson())
      ._("function", memberName)
      ._("arguments", args)
      ._("isVariation", variation)
      ._("fileRange", fileRange);
}

} // namespace qat::ast