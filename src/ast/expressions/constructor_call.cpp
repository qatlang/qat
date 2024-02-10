#include "./constructor_call.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/region.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IR::Value* ConstructorCall::emit(IR::Context* ctx) {
  SHOW("Constructor Call - Emitting type")
  if (!type.has_value() && !isTypeInferred()) {
    ctx->Error("No type provided for constructor call, and no type inferred from scope", fileRange);
  }
  auto* typ = type.has_value() ? type.value()->emit(ctx) : inferredType;
  if (type.has_value() && isTypeInferred()) {
    if (!typ->isSame(inferredType)) {
      ctx->Error("The provided type for the constructor call is " + ctx->highlightError(typ->toString()) +
                     " but the inferred type is " + ctx->highlightError(inferredType->toString()),
                 fileRange);
    }
  }
  SHOW("Emitted type")
  if (typ->isExpanded()) {
    auto*                                eTy = typ->asExpanded();
    Vec<IR::Value*>                      valsIR;
    Vec<Pair<Maybe<bool>, IR::QatType*>> valsType;
    for (auto* arg : args) {
      auto* argVal = arg->emit(ctx);
      valsType.push_back({argVal->isImplicitPointer() ? Maybe<bool>(argVal->isVariable()) : None, argVal->getType()});
      valsIR.push_back(argVal);
    }
    SHOW("Argument values emitted for function call")
    IR::MemberFunction* cons = nullptr;
    if (args.empty()) {
      if (eTy->hasDefaultConstructor()) {
        cons = eTy->getDefaultConstructor();
        if (!cons->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("The default constructor of type " + ctx->highlightError(eTy->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
        SHOW("Found default constructor")
      } else {
        ctx->Error(
            "Type " + ctx->highlightError(eTy->toString()) +
                " does not have a default constructor and hence arguments have to be provided in this constructor call",
            fileRange);
      }
    } else if (args.size() == 1) {
      if (eTy->hasFromConvertor(valsType[0].first, valsType[0].second)) {
        cons = eTy->getFromConvertor(valsType[0].first, valsType[0].second);
        if (!cons->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("This convertor of type " + ctx->highlightError(eTy->getFullName()) + " is not accessible here",
                     fileRange);
        }
        SHOW("Found convertor with type")
      } else {
        ctx->Error("No from convertor found for type " + ctx->highlightError(eTy->getFullName()) + " with type " +
                       ctx->highlightError(valsType.front().second->toString()),
                   fileRange);
      }
    } else {
      if (eTy->hasConstructorWithTypes(valsType)) {
        cons = eTy->getConstructorWithTypes(valsType);
        if (!cons->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("This constructor of type " + ctx->highlightError(eTy->getFullName()) + " is not accessible here",
                     fileRange);
        }
        SHOW("Found constructor with types")
      } else {
        ctx->Error("No matching constructor found for type " + ctx->highlightError(eTy->getFullName()), fileRange);
      }
    }
    SHOW("Found convertor/constructor")
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    auto argTys = cons->getType()->asFunction()->getArgumentTypes();
    for (usize i = 1; i < argTys.size(); i++) {
      if (argTys.at(i)->getType()->isReference()) {
        if (!valsType.at(i - 1).second->isReference()) {
          if (!valsType.at(i - 1).first.has_value()) {
            valsIR[i = 1] = valsIR[i - 1]->makeLocal(ctx, None, args[i - 1]->fileRange);
          }
          if (argTys.at(i)->getType()->asReference()->isSubtypeVariable() && !valsIR.at(i - 1)->isVariable()) {
            ctx->Error("The expected argument type is " + ctx->highlightError(argTys.at(i)->getType()->toString()) +
                           " but the provided value is not a variable",
                       args.at(i - 1)->fileRange);
          }
        } else {
          if (argTys.at(i)->getType()->asReference()->isSubtypeVariable() &&
              !valsIR.at(i - 1)->getType()->asReference()->isSubtypeVariable()) {
            ctx->Error("The expected argument type is " + ctx->highlightError(argTys.at(i)->getType()->toString()) +
                           " but the provided value is of type " +
                           ctx->highlightError(valsIR.at(i - 1)->getType()->toString()),
                       args.at(i - 1)->fileRange);
          }
        }
      } else {
        auto* valTy = valsType.at(i - 1).second;
        auto* val   = valsIR.at(i - 1);
        if (valTy->isReference() || val->isImplicitPointer()) {
          if (valTy->isReference()) {
            valTy = valTy->asReference()->getSubType();
          }
          valsIR.at(i - 1) = new IR::Value(ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM()), valTy, false,
                                           IR::Nature::temporary);
        }
      }
    }
    SHOW("About to create llAlloca")
    llvm::Value* llAlloca;
    if (isLocalDecl()) {
      llAlloca = localValue->getAlloca();
    } else {
      auto newAlloca = ctx->getActiveFunction()->getBlock()->newValue(
          irName.has_value() ? irName.value().value : ctx->getActiveFunction()->getRandomAllocaName(), eTy, isVar,
          irName.has_value() ? irName.value().range : fileRange);
      llAlloca = newAlloca->getLLVM();
    }
    Vec<llvm::Value*> valsLLVM;
    valsLLVM.push_back(llAlloca);
    for (auto* val : valsIR) {
      valsLLVM.push_back(val->getLLVM());
    }
    (void)cons->call(ctx, valsLLVM, None, ctx->getMod());
    if (isLocalDecl()) {
      return localValue->toNewIRValue();
    } else {
      auto* res = new IR::Value(llAlloca, eTy, irName.has_value() ? isVar : true, IR::Nature::temporary);
      if (isLocalDecl()) {
        res->setLocalID(localValue->getLocalID().value());
      }
      return res;
    }
  } else {
    ctx->Error("The provided type " + ctx->highlightError(typ->toString()) + " cannot be constructed",
               type.has_value() ? type.value()->fileRange : fileRange);
  }
  return nullptr;
}

Json ConstructorCall::toJson() const {
  Vec<JsonValue> argsJson;
  for (auto* arg : args) {
    argsJson.push_back(arg->toJson());
  }
  return Json()
      ._("nodeType", "constructorCall")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("arguments", argsJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast