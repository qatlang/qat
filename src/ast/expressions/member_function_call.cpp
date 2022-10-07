#include "./member_function_call.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "default.hpp"

namespace qat::ast {

IR::Value* MemberFunctionCall::emit(IR::Context* ctx) {
  SHOW("Member variable emitting")
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->getType();
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
      ctx->Error("The expression type has to be a pointer to use > to access members", instance->fileRange);
    }
  } else {
    if (instType->isPointer()) {
      ctx->Error(String("The expression is of pointer type. Please use ") + (variation ? ">~" : ">-") +
                     " to access member function",
                 instance->fileRange);
    }
  }
  if (instType->isCoreType()) {
    if (memberName == "end") {
      if (!instType->asCore()->hasDestructor()) {
        ctx->Error("Core type " + ctx->highlightError(instType->asCore()->getFullName()) +
                       " does not have a destructor",
                   fileRange);
      }
      auto* desFn = instType->asCore()->getDestructor();
      if (inst->isImplicitPointer() || inst->isReference()) {
        return desFn->call(ctx, {inst->getLLVM()}, ctx->getMod());
      } else {
        ctx->Error("Invalid expression to call the destructor of", fileRange);
      }
    }
    if (!instType->asCore()->hasMemberFunction(memberName)) {
      ctx->Error("Core type " + ctx->highlightError(instType->asCore()->toString()) +
                     " does not have a member function named " + ctx->highlightError(memberName) +
                     ". Please check the logic",
                 fileRange);
    }
    auto* cTy   = instType->asCore();
    auto* memFn = cTy->getMemberFunction(memberName);
    if (variation) {
      if (!memFn->isVariationFunction()) {
        ctx->Error("Member function " + ctx->highlightError(memberName) + " of core type " +
                       ctx->highlightError(cTy->getFullName()) +
                       " is not a variation and hence cannot be called as a variation",
                   fileRange);
      }
      if (!isVar) {
        ctx->Error("The expression does not have variability and hence variation "
                   "member functions cannot be called",
                   instance->fileRange);
      }
    } else {
      if (memFn->isVariationFunction()) {
        ctx->Error("Member function " + ctx->highlightError(memberName) + " of core type " +
                       ctx->highlightError(cTy->getFullName()) +
                       " is a variation and hence should be called as a variation",
                   fileRange);
      }
    }
    if (!memFn->isAccessible(ctx->getReqInfo())) {
      ctx->Error("Member function " + ctx->highlightError(memberName) + " of core type " +
                     ctx->highlightError(cTy->getFullName()) + " is not accessible here",
                 fileRange);
    }
    if (!inst->isImplicitPointer() && !inst->getType()->isReference() && !inst->getType()->isPointer()) {
      inst = inst->createAlloca(ctx->builder);
    }
    //
    auto fnArgsTy = memFn->getType()->asFunction()->getArgumentTypes();
    if ((fnArgsTy.size() - 1) != arguments.size()) {
      ctx->Error("Number of arguments provided for the member function call does not "
                 "match the signature",
                 fileRange);
    }
    Vec<IR::Value*> argsEmit;
    auto*           fnTy = memFn->getType()->asFunction();
    for (usize i = 0; i < arguments.size(); i++) {
      if (fnTy->getArgumentCount() > (u64)i) {
        auto* argTy = fnTy->getArgumentTypeAt(i + 1)->getType();
        if (arguments.at(i)->nodeType() == NodeType::integerLiteral) {
          if (argTy->isInteger() || argTy->isUnsignedInteger()) {
            ((IntegerLiteral*)arguments.at(i))->setType(argTy);
          }
        } else if (arguments.at(i)->nodeType() == NodeType::unsignedLiteral) {
          if (argTy->isInteger() || argTy->isUnsignedInteger()) {
            ((UnsignedLiteral*)arguments.at(i))->setType(argTy);
          }
        } else if (arguments.at(i)->nodeType() == NodeType::nullPointer) {
          if (argTy->isPointer()) {
            ((NullPointer*)arguments.at(i))
                ->setType(argTy->asPointer()->isSubtypeVariable(), argTy->asPointer()->getSubType());
          } else {
            ctx->Error("The expression provided does not match the type of the argument", arguments.at(i)->fileRange);
          }
        } else if (arguments.at(i)->nodeType() == NodeType::Default) {
          ((Default*)arguments.at(i))->setType(argTy);
        }
      }
      argsEmit.push_back(arguments.at(i)->emit(ctx));
    }
    SHOW("Argument values generated for member function")
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      if (!fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i - 1)->getType()) &&
          !(argsEmit.at(i - 1)->getType()->isReference() &&
            fnArgsTy.at(i)->getType()->isSame(argsEmit.at(i - 1)->getType()->asReference()->getSubType()))) {
        ctx->Error("Type of this expression does not match the type of the "
                   "corresponding argument of the function " +
                       ctx->highlightError(memFn->getName()),
                   arguments.at(i - 1)->fileRange);
      }
    }
    //
    Vec<llvm::Value*> argVals;
    argVals.push_back(inst->getLLVM());
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      if (fnArgsTy.at(i)->getType()->isReference() && !argsEmit.at(i - 1)->isReference()) {
        if (!argsEmit.at(i - 1)->isImplicitPointer()) {
          ctx->Error("Cannot pass a value for the argument that expects a reference", arguments.at(i - 1)->fileRange);
        }
      } else if (argsEmit.at(i - 1)->isReference()) {
        SHOW("Loading ref arg at " << i - 1 << " with type " << argsEmit.at(i - 1)->getType()->toString())
        argsEmit.at(i - 1) = new IR::Value(
            ctx->builder.CreateLoad(argsEmit.at(i - 1)->getType()->asReference()->getSubType()->getLLVMType(),
                                    argsEmit.at(i - 1)->getLLVM()),
            argsEmit.at(i - 1)->getType(), argsEmit.at(i - 1)->getType()->asReference()->isSubtypeVariable(),
            argsEmit.at(i - 1)->getNature());
      } else {
        argsEmit.at(i - 1)->loadImplicitPointer(ctx->builder);
      }
      argVals.push_back(argsEmit.at(i - 1)->getLLVM());
    }
    return memFn->call(ctx, argVals, ctx->getMod());
  } else {
    ctx->Error("Member function call for this type is not supported", fileRange);
  }
  return nullptr;
}

Json MemberFunctionCall::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    args.emplace_back(arg->toJson());
  }
  return Json()
      ._("nodeType", "memberFunctionCall")
      ._("instance", instance->toJson())
      ._("function", memberName)
      ._("arguments", args)
      ._("isVariation", variation)
      ._("fileRange", fileRange);
}

} // namespace qat::ast