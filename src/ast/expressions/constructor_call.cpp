#include "./constructor_call.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ConstructorCall::ConstructorCall(QatType* _type, Vec<Expression*> _exps, bool isHeap, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), args(std::move(_exps)), isHeaped(isHeap) {}

IR::Value* ConstructorCall::emit(IR::Context* ctx) {
  auto* typ = type->emit(ctx);
  if (typ->isCoreType()) {
    auto*             cTy = typ->asCore();
    Vec<IR::Value*>   valsIR;
    Vec<IR::QatType*> valsType;
    // FIXME - Support default constructor calls
    for (auto* arg : args) {
      auto* argVal = arg->emit(ctx);
      valsType.push_back(argVal->getType());
      valsIR.push_back(argVal);
    }
    SHOW("Argument values emitted for function call")
    IR::MemberFunction* cons = nullptr;
    if (args.size() == 1) {
      if (cTy->hasFromConvertor(valsType.front())) {
        cons = cTy->getFromConvertor(valsType.front());
        if (!cons->isAccessible(ctx->getReqInfo())) {
          ctx->Error("This convertor of core type " + ctx->highlightError(cTy->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
        SHOW("Found convertor with type")
      } else {
        ctx->Error("No from convertor found for core type " + ctx->highlightError(cTy->getFullName()) + " with type " +
                       ctx->highlightError(valsType.front()->toString()),
                   fileRange);
      }
    } else {
      if (cTy->hasConstructorWithTypes(valsType)) {
        cons = cTy->getConstructorWithTypes(valsType);
        if (!cons->isAccessible(ctx->getReqInfo())) {
          ctx->Error("This constructor of core type " + ctx->highlightError(cTy->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
        SHOW("Found constructor with types")
      } else {
        ctx->Error("No matching constructor found for core type " + ctx->highlightError(cTy->getFullName()), fileRange);
      }
    }
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    auto argTys = cons->getType()->asFunction()->getArgumentTypes();
    for (usize i = 1; i < argTys.size(); i++) {
      if (argTys.at(i)->getType()->isReference()) {
        if (!valsType.at(i - 1)->isReference()) {
          if (!valsIR.at(i - 1)->isImplicitPointer()) {
            valsIR.at(i - 1) = valsIR.at(i - 1)->createAlloca(ctx->builder);
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
        auto* valTy = valsType.at(i - 1);
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
    llvm::Value* llAlloca;
    if (isHeaped) {
      ctx->getMod()->linkNative(IR::NativeUnit::malloc);
      auto* mallocFn = ctx->getMod()->getLLVMModule()->getFunction("malloc");
      llAlloca =
          ctx->builder.CreatePointerCast(ctx->builder.CreateCall(mallocFn->getFunctionType(), mallocFn,
                                                                 {llvm::ConstantExpr::getSizeOf(cTy->getLLVMType())}),
                                         cTy->getLLVMType()->getPointerTo());
    } else {
      if (local) {
        if (local->getType()->isMaybe()) {
          llAlloca = ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u);
        } else {
          llAlloca = local->getAlloca();
        }
      } else if (!irName.empty()) {
        local    = ctx->fn->getBlock()->newValue(irName, cTy, isVar);
        llAlloca = local->getAlloca();
      } else {
        SHOW("Creating alloca for core type")
        llAlloca = ctx->builder.CreateAlloca(cTy->getLLVMType(), 0u);
      }
    }
    Vec<llvm::Value*> valsLLVM;
    valsLLVM.push_back(llAlloca);
    for (auto* val : valsIR) {
      valsLLVM.push_back(val->getLLVM());
    }
    (void)cons->call(ctx, valsLLVM, ctx->getMod());
    if (local && local->getType()->isMaybe()) {
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
                               ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u));
      auto* res = new IR::Value(local->getAlloca(), local->getType(), local->isVariable(), local->getNature());
      res->setLocalID(local->getLocalID());
      return res;
    } else {
      auto* res = new IR::Value(llAlloca, isHeaped ? (IR::QatType*)IR::PointerType::get(isVar, cTy, ctx->llctx) : cTy,
                                isHeaped ? false : isVar, IR::Nature::temporary);
      if (local) {
        res->setLocalID(local->getLocalID());
      }
      return res;
    }
  } else {
    ctx->Error("The provided type " + ctx->highlightError(typ->toString()) +
                   " is not a core type and hence the constructor cannot be "
                   "called for this type",
               type->fileRange);
  }
}

Json ConstructorCall::toJson() const {
  Vec<JsonValue> argsJson;
  for (auto* arg : args) {
    argsJson.push_back(arg->toJson());
  }
  return Json()
      ._("nodeType", "constructorCall")
      ._("type", type->toJson())
      ._("arguments", argsJson)
      ._("isHeap", isHeaped)
      ._("fileRange", fileRange);
}

} // namespace qat::ast