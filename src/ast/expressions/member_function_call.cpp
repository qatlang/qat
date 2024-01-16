#include "./member_function_call.hpp"

namespace qat::ast {

IR::Value* MemberFunctionCall::emit(IR::Context* ctx) {
  SHOW("Member variable emitting")
  if (isExpSelf) {
    if (ctx->getActiveFunction()->isMemberFunction()) {
      auto* memFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (memFn->isStaticFunction()) {
        ctx->Error("This is a static member function and hence cannot call member function on the parent instance",
                   fileRange);
      }
    } else {
      ctx->Error(
          "The parent function is not a member function of any type and hence cannot call member functions on the parent instance",
          fileRange);
    }
  } else {
    if (instance->nodeType() == NodeType::SELF) {
      ctx->Error("Do not use this syntax for calling member functions. Use " +
                     ctx->highlightError("''" + memberName.value + "(...)") + " instead",
                 fileRange);
    }
  }
  auto* inst     = isExpSelf ? ctx->getActiveFunction()->getFirstBlock()->getValue("''") : instance->emit(ctx);
  auto* instType = inst->getType();
  bool  isVar    = inst->isVariable();
  if (instType->isReference()) {
    inst->loadImplicitPointer(ctx->builder);
    isVar    = instType->asReference()->isSubtypeVariable();
    instType = instType->asReference()->getSubType();
  }
  if (instType->isPointer()) {
    ctx->Error("The expression is of pointer type. Please dereference the pointer to call the member function",
               instance->fileRange);
  }
  if (instType->isExpanded()) {
    if (memberName.value == "end") {
      if (isExpSelf) {
        ctx->Error("Cannot call the destructor on the parent instance. This is not allowed", fileRange);
      }
      if (!instType->asExpanded()->hasDestructor()) {
        ctx->Error("Type " + ctx->highlightError(instType->asExpanded()->getFullName()) + " does not have a destructor",
                   fileRange);
      }
      auto* desFn = instType->asExpanded()->getDestructor();
      if (!inst->isImplicitPointer() && !inst->isReference()) {
        inst = inst->makeLocal(ctx, None, instance->fileRange);
      } else if (inst->isReference()) {
        inst->loadImplicitPointer(ctx->builder);
      }
      return desFn->call(ctx, {inst->getLLVM()}, None, ctx->getMod());
    }
    auto* eTy = instType->asExpanded();
    if (callNature.has_value()) {
      if (callNature.value()) {
        if (!eTy->hasVariationFn(memberName.value)) {
          if (eTy->hasNormalMemberFn(memberName.value)) {
            ctx->Error(ctx->highlightError(memberName.value) + " is not a variation member function of type " +
                           ctx->highlightError(eTy->getFullName()) + " and hence cannot be called as a variation",
                       fileRange);
          } else {
            ctx->Error("No variation member function named " + ctx->highlightError(memberName.value) +
                           " found in type " + ctx->highlightError(eTy->getFullName()),
                       fileRange);
          }
        }
        if (!isVar) {
          ctx->Error("The expression does not have variability and hence variation "
                     "member functions cannot be called",
                     instance->fileRange);
        }
      } else {
        if (!eTy->hasNormalMemberFn(memberName.value)) {
          if (eTy->hasVariationFn(memberName.value)) {
            ctx->Error(ctx->highlightError(memberName.value) + " is a variation member function of type " +
                           ctx->highlightError(eTy->getFullName()) +
                           " and hence cannot be called as a normal member function",
                       fileRange);
          } else {
            ctx->Error("No normal member function named " + ctx->highlightError(memberName.value) + " found in type " +
                           ctx->highlightError(eTy->getFullName()),
                       fileRange);
          }
        }
      }
    } else {
      if (isVar) {
        if (!eTy->hasNormalMemberFn(memberName.value) && !eTy->hasVariationFn(memberName.value)) {
          ctx->Error("Type " + ctx->highlightError(instType->asExpanded()->toString()) +
                         " does not have a member function named " + ctx->highlightError(memberName.value) +
                         ". Please check the logic",
                     fileRange);
        }
      } else {
        if (!eTy->hasNormalMemberFn(memberName.value)) {
          if (eTy->hasVariationFn(memberName.value)) {
            ctx->Error(ctx->highlightError(memberName.value) + " is a variation member function of type " +
                           ctx->highlightError(eTy->getFullName()) + " and cannot be called here as this expression " +
                           (inst->isReference() ? "is a reference without variability" : "does not have variability"),
                       memberName.range);
          } else {
            ctx->Error("Type " + ctx->highlightError(instType->asExpanded()->toString()) +
                           " does not have a normal member function named " + ctx->highlightError(memberName.value) +
                           ". Please check the logic",
                       fileRange);
          }
        }
      }
    }
    auto* memFn = (((callNature.has_value() && callNature.value()) || isVar) && eTy->hasVariationFn(memberName.value))
                      ? eTy->getVariationFn(memberName.value)
                      : eTy->getNormalMemberFn(memberName.value);
    if (!memFn->isAccessible(ctx->getAccessInfo())) {
      ctx->Error("Member function " + ctx->highlightError(memberName.value) + " of type " +
                     ctx->highlightError(eTy->getFullName()) + " is not accessible here",
                 fileRange);
    }
    if (isExpSelf && instType->isCoreType() && ctx->getActiveFunction()->isMemberFunction()) {
      auto thisFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (thisFn->isConstructor()) {
        Vec<String> missingMembers;
        for (usize i = 0; i < instType->asCore()->getMemberCount(); i++) {
          if (!thisFn->isMemberInitted(i)) {
            missingMembers.push_back(instType->asCore()->getMemberNameAt(i));
          }
        }
        if (!missingMembers.empty()) {
          String message;
          for (usize i = 0; i < missingMembers.size(); i++) {
            message.append(ctx->highlightError(missingMembers[i]));
            if (i == missingMembers.size() - 2) {
              message.append(" and ");
            } else if (i + 1 < missingMembers.size()) {
              message.append(", ");
            }
          }
          // NOTE - Maybe consider changing this to deeper call-tree-analysis
          ctx->Error("Cannot call the " + String(callNature ? "variation " : "") + "member function as member field" +
                         (missingMembers.size() > 1 ? "s " : " ") + message +
                         " of this type have not been initialised yet. If the field" +
                         (missingMembers.size() > 1 ? "s or their" : " or its") +
                         " type have a default value, it will be used for initialisation only"
                         " at the end of this constructor",
                     fileRange);
        }
      }
      thisFn->addMemberFunctionCall(memFn);
    }
    if (!inst->isImplicitPointer() && !inst->getType()->isReference() && !inst->getType()->isPointer()) {
      inst = inst->makeLocal(ctx, None, instance->fileRange);
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
        if (arguments.at(i)->hasTypeInferrance()) {
          arguments.at(i)->asTypeInferrable()->setInferenceType(argTy);
        }
      }
      argsEmit.push_back(arguments.at(i)->emit(ctx));
    }
    SHOW("Argument values generated for member function")
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      auto fnArgType = fnArgsTy[i]->getType();
      auto argType   = argsEmit[i - 1]->getType();
      if (!(fnArgType->isSame(argType) || fnArgType->isCompatible(argType) ||
            (fnArgType->isReference() && argsEmit[i - 1]->isImplicitPointer() &&
             fnArgType->asReference()->getSubType()->isSame(argType)) ||
            (argType->isReference() && argType->asReference()->getSubType()->isSame(fnArgType)))) {
        ctx->Error("Type of this expression " + ctx->highlightError(argType->toString()) +
                       " does not match the type of the "
                       "corresponding argument " +
                       ctx->highlightError(memFn->getType()->asFunction()->getArgumentTypeAt(i)->getName()) +
                       " of the function " + ctx->highlightError(memFn->getFullName()),
                   arguments.at(i - 1)->fileRange);
      }
    }
    //
    Vec<llvm::Value*> argVals;
    Maybe<String>     localID = inst->getLocalID();
    argVals.push_back(inst->getLLVM());
    for (usize i = 1; i < fnArgsTy.size(); i++) {
      auto* currArg = argsEmit[i - 1];
      if (fnArgsTy.at(i)->getType()->isReference()) {
        auto fnRefTy = fnArgsTy[i]->getType()->asReference();
        if (currArg->isReference()) {
          if (fnRefTy->isSubtypeVariable() && !currArg->getType()->asReference()->isSubtypeVariable()) {
            ctx->Error("The type of the argument is " + ctx->highlightError(fnArgsTy[i]->getType()->toString()) +
                           " but the expression is of type " + ctx->highlightError(currArg->getType()->toString()),
                       arguments.at(i - 1)->fileRange);
          }
          currArg->loadImplicitPointer(ctx->builder);
        } else if (!currArg->isImplicitPointer()) {
          ctx->Error("Cannot pass a value for the argument that expects a reference", arguments.at(i - 1)->fileRange);
        } else if (fnArgsTy.at(i)->getType()->asReference()->isSubtypeVariable()) {
          if (!currArg->isVariable()) {
            ctx->Error("The argument " + ctx->highlightError(fnArgsTy.at(i)->getName()) + " is of type " +
                           ctx->highlightError(fnArgsTy.at(i)->getType()->toString()) +
                           " which is a reference with variability, but this expression does not have variability",
                       arguments.at(i - 1)->fileRange);
          }
        }
      } else if (currArg->isReference() || currArg->isImplicitPointer()) {
        if (currArg->isReference()) {
          currArg->loadImplicitPointer(ctx->builder);
        }
        SHOW("Loading ref arg at " << i - 1 << " with type " << currArg->getType()->toString())
        auto* argTy    = fnArgsTy[i]->getType();
        auto* argValTy = currArg->getType();
        auto  isRefVar =
            currArg->isReference() ? currArg->getType()->asReference()->isSubtypeVariable() : currArg->isVariable();
        if (argTy->isTriviallyCopyable() || argTy->isTriviallyMovable()) {
          auto* argEmitLLVMValue = currArg->getLLVM();
          argsEmit[i - 1] = new IR::Value(ctx->builder.CreateLoad(argTy->getLLVMType(), currArg->getLLVM()), argTy,
                                          false, IR::Nature::temporary);
          if (!argTy->isTriviallyMovable()) {
            if (!isRefVar) {
              if (argValTy->isReference()) {
                ctx->Error("This expression is a reference without variability and hence cannot be trivilly moved from",
                           arguments[i - 1]->fileRange);
              } else {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           arguments[i - 1]->fileRange);
              }
            }
            ctx->builder.CreateStore(llvm::Constant::getNullValue(argTy->getLLVMType()), argEmitLLVMValue);
          }
        } else {
          ctx->Error("This expression is not a value and is of type " + ctx->highlightError(argTy->toString()) +
                         " which is not trivially copyable or trivially movable. Please use " +
                         ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " instead",
                     arguments[i - 1]->fileRange);
        }
      }
      SHOW("Argument value at " << i - 1 << " is of type " << argsEmit[i - 1]->getType()->toString()
                                << " and argtype is " << fnArgsTy.at(i)->getType()->toString())
      argVals.push_back(argsEmit[i - 1]->getLLVM());
    }
    return memFn->call(ctx, argVals, localID, ctx->getMod());
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
      ._("hasCallNature", callNature.has_value())
      ._("callNature", callNature.has_value() ? callNature.value() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast