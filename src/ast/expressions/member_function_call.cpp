#include "./member_function_call.hpp"
#include "../../IR/qat_module.hpp"
#include "../../IR/types/vector.hpp"
#include "../prerun/member_function_call.hpp"
#include "llvm/IR/Intrinsics.h"

namespace qat::ast {

void MemberFunctionCall::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                                             IR::Context* ctx) {
  UPDATE_DEPS(instance);
  for (auto arg : arguments) {
    UPDATE_DEPS(arg);
  }
  for (auto mod : IR::QatModule::allModules) {
    for (auto modEnt : mod->entityEntries) {
      if (modEnt->type == IR::EntityType::defaultDoneSkill) {
        if (modEnt->has_child(memberName.value)) {
          ent->addDependency(IR::EntityDependency{modEnt, IR::DependType::partial, phase});
        }
      } else if (modEnt->type == IR::EntityType::structType) {
        if (modEnt->has_child(memberName.value)) {
          ent->addDependency(IR::EntityDependency{modEnt, IR::DependType::childrenPartial, phase});
        }
      }
    }
  }
}

IR::Value* MemberFunctionCall::emit(IR::Context* ctx) {
  SHOW("Member variable emitting")
  if (isExpSelf) {
    if (ctx->getActiveFunction()->isMemberFunction()) {
      auto* memFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (memFn->isStaticFunction()) {
        ctx->Error("This is a static method and hence cannot call method on the parent instance", fileRange);
      }
    } else {
      ctx->Error("The parent function is not a method of any type and hence cannot call methods on the parent instance",
                 fileRange);
    }
  } else {
    if (instance->nodeType() == NodeType::SELF) {
      ctx->Error("Do not use this syntax for calling methods. Use " +
                     ctx->highlightError("''" + memberName.value + "(...)") + " instead",
                 fileRange);
    }
  }
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->getType();
  bool  isVar    = inst->isVariable();
  if (instType->isReference()) {
    inst->loadImplicitPointer(ctx->builder);
    isVar    = instType->asReference()->isSubtypeVariable();
    instType = instType->asReference()->getSubType();
  }
  if (instType->isPointer()) {
    ctx->Error("The expression is of pointer type. Please dereference the pointer to call the method",
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
            ctx->Error(ctx->highlightError(memberName.value) + " is not a variation method of type " +
                           ctx->highlightError(eTy->getFullName()) + " and hence cannot be called as a variation",
                       fileRange);
          } else {
            ctx->Error("No variation method named " + ctx->highlightError(memberName.value) + " found in type " +
                           ctx->highlightError(eTy->getFullName()),
                       fileRange);
          }
        }
        if (!isVar) {
          ctx->Error("The expression does not have variability and hence variation methods cannot be called",
                     instance->fileRange);
        }
      } else {
        if (!eTy->hasNormalMemberFn(memberName.value)) {
          if (eTy->hasVariationFn(memberName.value)) {
            ctx->Error(ctx->highlightError(memberName.value) + " is a variation method of type " +
                           ctx->highlightError(eTy->getFullName()) + " and hence cannot be called as a normal method",
                       fileRange);
          } else {
            ctx->Error("No normal method named " + ctx->highlightError(memberName.value) + " found in type " +
                           ctx->highlightError(eTy->getFullName()),
                       fileRange);
          }
        }
      }
    } else {
      if (isVar) {
        if (!eTy->has_valued_method(memberName.value) && !eTy->hasNormalMemberFn(memberName.value) &&
            !eTy->hasVariationFn(memberName.value)) {
          ctx->Error("Type " + ctx->highlightError(instType->asExpanded()->toString()) +
                         " does not have a method named " + ctx->highlightError(memberName.value) +
                         ". Please check the logic",
                     fileRange);
        }
      } else {
        if (!eTy->has_valued_method(memberName.value)) {
          if (!eTy->hasNormalMemberFn(memberName.value)) {
            if (eTy->hasVariationFn(memberName.value)) {
              ctx->Error(ctx->highlightError(memberName.value) + " is a variation method of type " +
                             ctx->highlightError(eTy->getFullName()) +
                             " and cannot be called here as this expression " +
                             (inst->isReference() ? "is a reference without variability" : "does not have variability"),
                         memberName.range);
            } else {
              ctx->Error("Type " + ctx->highlightError(instType->asExpanded()->toString()) +
                             " does not have a method named " + ctx->highlightError(memberName.value) +
                             ". Please check the logic",
                         fileRange);
            }
          }
        }
      }
    }
    auto* memFn = (((callNature.has_value() && callNature.value()) || isVar) && eTy->hasVariationFn(memberName.value))
                      ? eTy->getVariationFn(memberName.value)
                      : (eTy->has_valued_method(memberName.value) ? eTy->get_valued_method(memberName.value)
                                                                  : eTy->getNormalMemberFn(memberName.value));
    if (!memFn->isAccessible(ctx->getAccessInfo())) {
      ctx->Error("Method " + ctx->highlightError(memberName.value) + " of type " +
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
          ctx->Error("Cannot call the " + String(callNature ? "variation " : "") + "method as member field" +
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
    if (!inst->isImplicitPointer() && !inst->isReference() &&
        (memFn->getMemberFnType() != IR::MemberFnType::value_method)) {
      inst = inst->makeLocal(ctx, None, instance->fileRange);
    } else if ((memFn->getMemberFnType() == IR::MemberFnType::value_method) &&
               (inst->isImplicitPointer() || inst->isReference())) {
      if (instType->isTriviallyCopyable() || instType->isTriviallyMovable()) {
        if (inst->isReference()) {
          inst->loadImplicitPointer(ctx->builder);
        }
        auto origInst = inst;
        inst          = new IR::Value(ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM()), instType, true,
                                      IR::Nature::temporary);
        if (!instType->isTriviallyCopyable()) {
          if (inst->isReference() && !inst->getType()->asReference()->isSubtypeVariable()) {
            ctx->Error("This is a reference without variability and hence cannot be trivially moved from",
                       instance->fileRange);
          } else if (inst->isImplicitPointer() && !inst->isVariable()) {
            ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                       instance->fileRange);
          }
          ctx->builder.CreateStore(llvm::Constant::getNullValue(instType->getLLVMType()), origInst->getLLVM());
        }
      } else {
        ctx->Error(
            "The method to be called is a value method, so it requires a value of the parent type. The parent type is " +
                ctx->highlightError(instType->toString()) + " which is not trivially copyable or movable. Please use " +
                ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
            fileRange);
      }
    }
    //
    auto fnArgsTy = memFn->getType()->asFunction()->getArgumentTypes();
    if ((fnArgsTy.size() - 1) != arguments.size()) {
      ctx->Error("Number of arguments provided for the method call does not "
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
    SHOW("Argument values generated for method")
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
  } else if (instType->isTyped()) {
    return handle_type_wrap_functions(instType->asTyped(), arguments, memberName, ctx, fileRange);
  } else if (instType->is_vector()) {
    auto*      origInst        = new IR::Value(inst->getLLVM(), inst->getType(), false, IR::Nature::temporary);
    const auto isOrigRefNotVar = (origInst->isImplicitPointer() && !origInst->isVariable()) ||
                                 (origInst->isReference() && !origInst->getType()->asReference()->isSubtypeVariable());
    const auto shouldStore = origInst->isImplicitPointer() || origInst->isReference();
    auto*      vecTy       = instType->as_vector();
    auto       refHandler  = [&]() {
      if (inst->isReference() || inst->isImplicitPointer()) {
        inst->loadImplicitPointer(ctx->builder);
        if (inst->isReference()) {
          inst = new IR::Value(ctx->builder.CreateLoad(vecTy->getLLVMType(), inst->getLLVM()), vecTy, false,
                                      IR::Nature::temporary);
        }
      }
    };
    if (memberName.value == "insert") {
      if (isOrigRefNotVar) {
        ctx->Error(
            "This " + String(origInst->isImplicitPointer() ? "expression" : "reference") +
                " does not have variability and hence cannot be inserted into. Inserting vectors to values are possible, but for clarity, make sure to copy the vector first using " +
                ctx->highlightError("'copy"),
            fileRange);
      }
      if (!vecTy->is_scalable()) {
        ctx->Error("Method " + ctx->highlightError(memberName.value) +
                       " can only be called on expressions with scalable vector type. This expression is of type " +
                       ctx->highlightError(vecTy->toString()) + " which is not scalable",
                   fileRange);
      }
      if (arguments.size() != 1u) {
        ctx->Error("Method " + ctx->highlightError(memberName.value) + " requires " +
                       (arguments.size() > 1 ? "only " : "") + "one argument of type " +
                       ctx->highlightError(vecTy->get_non_scalable_type(ctx)->toString()),
                   fileRange);
      }
      if (arguments[0]->hasTypeInferrance()) {
        arguments[0]->asTypeInferrable()->setInferenceType(vecTy->get_non_scalable_type(ctx));
      }
      auto* argVec = arguments[0]->emit(ctx);
      if (argVec->getType()->isSame(vecTy->get_non_scalable_type(ctx)) ||
          argVec->getType()->asReference()->getSubType()->isSame(vecTy->get_non_scalable_type(ctx))) {
        if (argVec->isReference() || argVec->isImplicitPointer()) {
          argVec->loadImplicitPointer(ctx->builder);
          if (argVec->isReference()) {
            argVec = new IR::Value(
                ctx->builder.CreateLoad(vecTy->get_non_scalable_type(ctx)->getLLVMType(), argVec->getLLVM()),
                vecTy->get_non_scalable_type(ctx), false, IR::Nature::temporary);
          }
        }
        refHandler();
        auto result = ctx->builder.CreateInsertVector(vecTy->getLLVMType(), inst->getLLVM(), argVec->getLLVM(),
                                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u));
        if (shouldStore) {
          if (origInst->isReference()) {
            origInst->loadImplicitPointer(ctx->builder);
          }
          ctx->builder.CreateStore(result, origInst->getLLVM());
          return origInst;
        } else {
          return new IR::Value(result, vecTy, false, IR::Nature::temporary);
        }
      } else {
        ctx->Error("The argument provided to " + ctx->highlightError(memberName.value) + " is expected to be of type " +
                       ctx->highlightError(vecTy->get_non_scalable_type(ctx)->toString()),
                   fileRange);
      }
    } else if (memberName.value == "reverse") {
      if (isOrigRefNotVar) {
        ctx->Error(
            "This " + String(origInst->isImplicitPointer() ? "expression" : "reference") +
                " does not have variability and hence cannot be reversed in-place. Reversing vector values are possible, but for clarity, make sure to copy the vector first using " +
                ctx->highlightError("'copy"),
            fileRange);
      }
      if (!arguments.empty()) {
        ctx->Error("The method " + ctx->highlightError(memberName.value) + " expects zero arguments, but " +
                       ctx->highlightError(std::to_string(arguments.size())) + " values were provided",
                   fileRange);
      }
      refHandler();
      auto result = ctx->builder.CreateVectorReverse(inst->getLLVM());
      if (shouldStore) {
        ctx->builder.CreateStore(result, origInst->getLLVM());
        return origInst;
      } else {
        return new IR::Value(result, vecTy, false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Method " + ctx->highlightError(memberName.value) + " is not supported for expression of type " +
                     ctx->highlightError(vecTy->toString()),
                 fileRange);
    }
  } else {
    ctx->Error("Method call for expression of type " + ctx->highlightError(instType->toString()) + " is not supported",
               fileRange);
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