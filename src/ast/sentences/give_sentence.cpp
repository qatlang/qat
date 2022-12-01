#include "./give_sentence.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../expressions/default.hpp"
#include "../expressions/none.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

GiveSentence::GiveSentence(Maybe<Expression*> _given_expr, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), give_expr(_given_expr) {}

IR::Value* GiveSentence::emit(IR::Context* ctx) {
  auto* fun = ctx->fn;
  if (fun->isAsyncFunction() ? fun->getType()
                                   ->asFunction()
                                   ->getReturnArgType()
                                   ->asReference()
                                   ->getSubType()
                                   ->asFuture()
                                   ->getSubType()
                                   ->isVoid()
                             : fun->getType()->asFunction()->getReturnType()->isVoid()) {
    if (fun->isMemberFunction() && ((IR::MemberFunction*)fun)->getMemberFnType() == IR::MemberFnType::destructor) {
      auto* mFn = (IR::MemberFunction*)fun;
      auto* cTy = mFn->getParentType();
      for (usize i = 0; i < cTy->getMemberCount(); i++) {
        if (cTy->getMemberAt(i)->type->isCoreType()) {
          auto* memPtr     = ctx->builder.CreateStructGEP(mFn->getParentType()->getLLVMType(), ctx->selfVal, i);
          auto* destructor = cTy->getDestructor();
          (void)destructor->call(ctx, {memPtr}, ctx->getMod());
        }
      }
    }
    Vec<IR::LocalValue*> locals;
    fun->getBlock()->collectAllLocalValuesSoFar(locals);
    for (auto* loc : locals) {
      if (loc->getType()->isCoreType()) {
        auto* cTy        = loc->getType()->asCore();
        auto* destructor = cTy->getDestructor();
        (void)destructor->call(ctx, {loc->getAlloca()}, ctx->getMod());
      }
    }
    locals.clear();
    if (fun->isAsyncFunction()) {
      auto* futureTy = fun->getType()->asFunction()->getReturnArgType()->asReference()->getSubType()->getLLVMType();
      auto* futRet =
          ctx->builder.CreateLoad(futureTy->getPointerTo(), fun->getBlock()->getValue("qat'future")->getAlloca());
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
                               ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                                       ctx->builder.CreateStructGEP(futureTy, futRet, 2u)));
      ctx->getMod()->linkNative(IR::NativeUnit::pthreadExit);
      auto* pthreadExit = ctx->getMod()->getLLVMModule()->getFunction("pthread_exit");
      ctx->builder.CreateCall(pthreadExit->getFunctionType(), pthreadExit,
                              {llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
      SHOW("Creating return for async function with void return")
      if (give_expr.has_value()) {
        auto* giveExpr = give_expr.value()->emit(ctx);
        if (!giveExpr->getType()->isVoid()) {
          ctx->Error("The return type of the function is " + ctx->highlightError("void") +
                         " but the expression is of type " + ctx->highlightError(giveExpr->getType()->toString()),
                     give_expr.value()->fileRange);
        }
      }
      return new IR::Value(
          ctx->builder.CreateRet(llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())),
          IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
    } else {
      if (give_expr.has_value()) {
        auto* giveExpr = give_expr.value()->emit(ctx);
        if (!giveExpr->getType()->isVoid()) {
          ctx->Error("The return type of the function is " + ctx->highlightError("void") +
                         " but the expression is of type " + ctx->highlightError(giveExpr->getType()->toString()),
                     give_expr.value()->fileRange);
        }
      }
      return new IR::Value(ctx->builder.CreateRetVoid(), IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
    }
  } else {
    if (give_expr.has_value()) {
      auto* retType = fun->isAsyncFunction()
                          ? fun->getType()->asFunction()->getReturnArgType()->asReference()->getSubType()
                          : fun->getType()->asFunction()->getReturnType();
      retType       = fun->isAsyncFunction() ? retType->asFuture()->getSubType() : retType;
      if (give_expr.value()->nodeType() == NodeType::Default) {
        ((Default*)give_expr.value())->setType(retType->isFuture() ? retType->asFuture()->getSubType() : retType);
      }
      if (retType->isInteger() || retType->isUnsignedInteger() ||
          (retType->isMaybe() &&
           (retType->asMaybe()->getSubType()->isUnsignedInteger() || retType->asMaybe()->getSubType()->isInteger())) ||
          (retType->isFuture() && ((retType->asFuture()->getSubType()->isUnsignedInteger() ||
                                    retType->asFuture()->getSubType()->isInteger()) ||
                                   (retType->asFuture()->getSubType()->isMaybe() &&
                                    (retType->asFuture()->getSubType()->asMaybe()->isUnsignedInteger() ||
                                     retType->asFuture()->getSubType()->asMaybe()->isInteger()))))) {
        if (give_expr.value()->nodeType() == NodeType::integerLiteral) {
          ((IntegerLiteral*)give_expr.value())
              ->setType(retType->isMaybe()
                            ? retType->asMaybe()->getSubType()
                            : (retType->isFuture() ? (retType->asFuture()->getSubType()->isMaybe()
                                                          ? retType->asFuture()->getSubType()->asMaybe()->getSubType()
                                                          : retType->asFuture()->getSubType())
                                                   : retType));
        } else if (give_expr.value()->nodeType() == NodeType::unsignedLiteral) {
          ((UnsignedLiteral*)give_expr.value())
              ->setType(retType->isMaybe()
                            ? retType->asMaybe()->getSubType()
                            : (retType->isFuture() ? (retType->asFuture()->getSubType()->isMaybe()
                                                          ? retType->asFuture()->getSubType()->asMaybe()->getSubType()
                                                          : retType->asFuture()->getSubType())
                                                   : retType));
        }
      } else if (retType->isPointer() || (retType->isMaybe() && retType->asMaybe()->getSubType()->isPointer()) ||
                 (retType->isFuture() && retType->asFuture()->getSubType()->isPointer()) ||
                 (retType->isFuture() && retType->asFuture()->getSubType()->isMaybe() &&
                  retType->asFuture()->getSubType()->asMaybe()->getSubType()->isPointer())) {
        if (give_expr.value()->nodeType() == NodeType::nullPointer) {
          ((NullPointer*)give_expr.value())
              ->setType(retType->isPointer()
                            ? retType->asPointer()
                            : (retType->asMaybe()
                                   ? (retType->asMaybe()->getSubType()->asPointer())
                                   : ((retType->isFuture() && retType->asFuture()->getSubType()->isMaybe())
                                          ? retType->asFuture()->getSubType()->asMaybe()->getSubType()->asPointer()
                                          : retType->asFuture()->getSubType()->asPointer())));
        }
      } else if (retType->isMaybe() || (retType->isFuture() && retType->asFuture()->getSubType()->isMaybe())) {
        if (give_expr.value()->nodeType() == NodeType::none) {
          ((NoneExpression*)give_expr.value())
              ->setType(retType->isMaybe() ? retType->asMaybe()->getSubType()
                                           : retType->asFuture()->getSubType()->asMaybe()->getSubType());
        }
      }
      auto* retVal = give_expr.value()->emit(ctx);
      SHOW("ret val emitted")
      if (retType->isSame(retVal->getType()) ||
          (retType->isReference() && retType->asReference()->getSubType()->isSame(retVal->getType()) &&
           retVal->isImplicitPointer()) ||
          (retVal->getType()->isReference() && retVal->getType()->asReference()->getSubType()->isSame(retType))) {
        SHOW("Return type is same")
        if (retVal->getType()->isReference() && !(retType->isReference())) {
          retVal = new IR::Value(ctx->builder.CreateLoad(retType->getLLVMType(), retVal->getLLVM()), retType, false,
                                 IR::Nature::temporary);
        } else if (retVal->isImplicitPointer() && !retType->isReference()) {
          retVal->loadImplicitPointer(ctx->builder);
        }
        if (retType->isReference() && retVal->isLocalToFn()) {
          ctx->Warning("Returning reference to a local value. The value "
                       "pointed to by the reference might be destroyed before "
                       "the function call is complete",
                       fileRange);
        }
        Vec<IR::LocalValue*> locals;
        fun->getBlock()->collectAllLocalValuesSoFar(locals);
        for (auto* loc : locals) {
          if (loc->getType()->isCoreType()) {
            auto* cTy        = loc->getType()->asCore();
            auto* destructor = cTy->getDestructor();
            (void)destructor->call(ctx, {loc->getAlloca()}, ctx->getMod());
          }
        }
        locals.clear();
        if (fun->isAsyncFunction()) {
          auto* futureTy = fun->getType()->asFunction()->getReturnArgType()->asReference()->getSubType()->asFuture();
          SHOW("Got future type")
          auto* futureRet = ctx->builder.CreateLoad(futureTy->getLLVMType()->getPointerTo(),
                                                    fun->getBlock()->getValue("qat'future")->getAlloca());
          SHOW("Got future ret")
          ctx->builder.CreateStore(
              retVal->getLLVM(),
              ctx->builder.CreateLoad(futureTy->getSubType()->getLLVMType()->getPointerTo(),
                                      ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRet, 3u)));
          SHOW("Stored value to future")
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
              ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                      ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRet, 2u)));
          SHOW("Stored tag")
          ctx->getMod()->linkNative(IR::NativeUnit::pthreadExit);
          auto* pthreadExit = ctx->getMod()->getLLVMModule()->getFunction("pthread_exit");
          ctx->builder.CreateCall(pthreadExit->getFunctionType(), pthreadExit,
                                  {llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
          return new IR::Value(ctx->builder.CreateRet(llvm::ConstantPointerNull::get(
                                   llvm::dyn_cast<llvm::PointerType>(fun->getAsyncSubFunction()->getReturnType()))),
                               IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
        } else {
          return new IR::Value(ctx->builder.CreateRet(retVal->getLLVM()), retVal->getType(), false, IR::Nature::pure);
        }
      } else if (fun->isAsyncFunction() && retType->isFuture() &&
                 (retType->asFuture()->getSubType()->isSame(retVal->getType()) ||
                  (retVal->isReference() &&
                   retType->asFuture()->getSubType()->isSame(retVal->getType()->asReference()->getSubType())) ||
                  (retType->asFuture()->getSubType()->isReference() &&
                   (retVal->isImplicitPointer() &&
                    (retType->asFuture()->getSubType()->asReference()->isSubtypeVariable() ? retVal->isVariable()
                                                                                           : true) &&
                    retType->asFuture()->getSubType()->asReference()->getSubType()->isSame(retVal->getType()))))) {
        SHOW("GIVE: Async fn and value type matches return type")
        // FIXME - Check returning of future values
        // FIXME - Implement copy & move semantics
        if (retType->asFuture()->getSubType()->isSame(retVal->getType())) {
          retVal->loadImplicitPointer(ctx->builder);
        } else if (retVal->isReference() &&
                   retType->asFuture()->getSubType()->isSame(retVal->getType()->asReference()->getSubType())) {
          retVal = new IR::Value(
              ctx->builder.CreateLoad(retVal->getType()->asReference()->getSubType()->getLLVMType(), retVal->getLLVM()),
              retVal->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
        }
        auto* futureTy  = fun->getType()->asFunction()->getReturnArgType()->asReference()->getSubType()->asFuture();
        auto* futureRet = ctx->builder.CreateLoad(futureTy->getLLVMType()->getPointerTo(),
                                                  fun->getBlock()->getValue("qat'future")->getAlloca());
        ctx->builder.CreateStore(
            retVal->getLLVM(),
            ctx->builder.CreateLoad(futureTy->getSubType()->getLLVMType()->getPointerTo(),
                                    ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRet, 3u)),
            true);
        ctx->builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
            ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                    ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRet, 2u)),
            true);
        ctx->getMod()->linkNative(IR::NativeUnit::pthreadExit);
        auto* pthreadExit = ctx->getMod()->getLLVMModule()->getFunction("pthread_exit");
        ctx->builder.CreateCall(pthreadExit->getFunctionType(), pthreadExit,
                                {llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
        return new IR::Value(
            ctx->builder.CreateRet(llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())),
            IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
      } else {
        ctx->Error("Given value type of the function is " + fun->getType()->asFunction()->getReturnType()->toString() +
                       ", but the provided value in the give sentence is " + retVal->getType()->toString(),
                   give_expr.value()->fileRange);
      }
    } else {
      ctx->Error("No value is provided for the give sentence. Please provide a "
                 "value of the appropriate type",
                 fileRange);
    }
  }
}

Json GiveSentence::toJson() const {
  return Json()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value", give_expr.has_value() ? give_expr.value()->toJson() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast