#include "./constructor_call.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ConstructorCall::ConstructorCall(QatType *_type, Vec<Expression *> _exps,
                                 bool isHeap, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), args(std::move(_exps)),
      isHeaped(isHeap) {}

IR::Value *ConstructorCall::emit(IR::Context *ctx) {
  auto *typ = type->emit(ctx);
  if (typ->isCoreType()) {
    auto              *cTy = typ->asCore();
    Vec<IR::Value *>   valsIR;
    Vec<IR::QatType *> valsType;
    // FIXME - Support default constructor calls
    for (auto *arg : args) {
      auto *argVal = arg->emit(ctx);
      valsType.push_back(argVal->getType());
      valsIR.push_back(argVal);
    }
    SHOW("Argument values emitted for function call")
    IR::MemberFunction *cons = nullptr;
    if (args.size() == 1) {
      if (cTy->hasFromConvertor(valsType.front())) {
        cons = cTy->getFromConvertor(valsType.front());
        if (!cons->isAccessible(ctx->getReqInfo())) {
          ctx->Error("This convertor of core type " +
                         ctx->highlightError(cTy->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
        SHOW("Found convertor with type")
      } else {
        ctx->Error("No from convertor found for core type " +
                       ctx->highlightError(cTy->getFullName()) + " with type " +
                       ctx->highlightError(valsType.front()->toString()),
                   fileRange);
      }
    } else {
      if (cTy->hasConstructorWithTypes(valsType)) {
        cons = cTy->getConstructorWithTypes(valsType);
        if (!cons->isAccessible(ctx->getReqInfo())) {
          ctx->Error("This constructor of core type " +
                         ctx->highlightError(cTy->getFullName()) +
                         " is not accessible here",
                     fileRange);
        }
        SHOW("Found constructor with types")
      } else {
        ctx->Error("No matching constructor found for core type " +
                       ctx->highlightError(cTy->getFullName()),
                   fileRange);
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
          if (argTys.at(i)->getType()->asReference()->isSubtypeVariable() &&
              !valsIR.at(i - 1)->isVariable()) {
            ctx->Error(
                "The expected argument type is " +
                    ctx->highlightError(argTys.at(i)->getType()->toString()) +
                    " but the provided value is not a variable",
                args.at(i - 1)->fileRange);
          }
        } else {
          if (argTys.at(i)->getType()->asReference()->isSubtypeVariable() &&
              !valsIR.at(i - 1)
                   ->getType()
                   ->asReference()
                   ->isSubtypeVariable()) {
            ctx->Error(
                "The expected argument type is " +
                    ctx->highlightError(argTys.at(i)->getType()->toString()) +
                    " but the provided value is of type " +
                    ctx->highlightError(
                        valsIR.at(i - 1)->getType()->toString()),
                args.at(i - 1)->fileRange);
          }
        }
      } else {
        auto *valTy = valsType.at(i - 1);
        auto *val   = valsIR.at(i - 1);
        if (valTy->isReference() || val->isImplicitPointer()) {
          if (valTy->isReference()) {
            valTy = valTy->asReference()->getSubType();
          }
          valsIR.at(i - 1) = new IR::Value(
              ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM()),
              valTy, false, IR::Nature::temporary);
        }
      }
    }
    llvm::Value *llAlloca;
    if (isHeaped) {
      ctx->getMod()->linkNative(IR::NativeUnit::malloc);
      auto *mallocFn = ctx->getMod()->getLLVMModule()->getFunction("malloc");
      llAlloca       = ctx->builder.CreatePointerCast(
          ctx->builder.CreateCall(
              mallocFn->getFunctionType(), mallocFn,
              {llvm::ConstantExpr::getSizeOf(cTy->getLLVMType())}),
          cTy->getLLVMType()->getPointerTo());
    } else {
      if (local) {
        llAlloca = local->getAlloca();
      } else if (!irName.empty()) {
        local    = ctx->fn->getBlock()->newValue(irName, cTy, isVar);
        llAlloca = local->getAlloca();
      } else {
        SHOW("Creating alloca for core type")
        llAlloca = ctx->builder.CreateAlloca(cTy->getLLVMType(), 0u);
      }
    }
    Vec<llvm::Value *> valsLLVM;
    valsLLVM.push_back(llAlloca);
    for (auto *val : valsIR) {
      valsLLVM.push_back(val->getLLVM());
    }
    (void)cons->call(ctx, valsLLVM, ctx->getMod());
    return new IR::Value(
        llAlloca,
        isHeaped
            ? (IR::QatType *)IR::PointerType::get(isVar, cTy, ctx->llctx)
            : (IR::QatType *)IR::ReferenceType::get(isVar, cTy, ctx->llctx),
        false, IR::Nature::temporary);
  } else if (typ->isStringSlice()) {
    if (args.size() == 2) {
      auto *strData  = args.at(0)->emit(ctx);
      auto *strLen   = args.at(1)->emit(ctx);
      auto *dataType = strData->getType();
      auto *lenType  = strLen->getType();
      if (dataType->isReference()) {
        dataType = dataType->asReference()->getSubType();
      }
      if (lenType->isReference()) {
        lenType = lenType->asReference()->getSubType();
      }
      if (dataType->isPointer() &&
          dataType->asPointer()->getSubType()->isUnsignedInteger() &&
          dataType->asPointer()
                  ->getSubType()
                  ->asUnsignedInteger()
                  ->getBitwidth() == 8u) {
        if (strData->isImplicitPointer() || strData->getType()->isReference()) {
          strData =
              new IR::Value(ctx->builder.CreateLoad(dataType->getLLVMType(),
                                                    strData->getLLVM()),
                            dataType, false, IR::Nature::temporary);
        }
        if (lenType->isUnsignedInteger() &&
            lenType->asUnsignedInteger()->getBitwidth() == 64u) {
          if (strLen->isImplicitPointer() || strLen->isReference()) {
            strLen =
                new IR::Value(ctx->builder.CreateLoad(lenType->getLLVMType(),
                                                      strLen->getLLVM()),
                              lenType, false, IR::Nature::temporary);
          }
          auto *strSliceTy = IR::StringSliceType::get(ctx->llctx);
          auto *strAlloca =
              ctx->builder.CreateAlloca(strSliceTy->getLLVMType());
          ctx->builder.CreateStore(
              strData->getLLVM(), ctx->builder.CreateStructGEP(
                                      strSliceTy->getLLVMType(), strAlloca, 0));
          ctx->builder.CreateStore(
              strLen->getLLVM(), ctx->builder.CreateStructGEP(
                                     strSliceTy->getLLVMType(), strAlloca, 1));
          return new IR::Value(strAlloca, strSliceTy, false, IR::Nature::pure);
        } else {
          ctx->Error("The second argument for creating a string slice is not a "
                     "64-bit unsigned integer",
                     args.at(1)->fileRange);
        }
      } else {
        ctx->Error("The first argument for creating a string slice is not a "
                   "pointer to an unsigned 8-bit integer",
                   args.at(0)->fileRange);
      }
    } else {
      ctx->Error(
          "Creating a string slice using constructor requires 2 arguments. The "
          "first argument should be the pointer to the start of the data and "
          "the second argument should be the length of the data (including the "
          "terminating null character).",
          fileRange);
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
  for (auto *arg : args) {
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