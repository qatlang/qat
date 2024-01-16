#include "./give_sentence.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

IR::Value* GiveSentence::emit(IR::Context* ctx) {
  auto* fun = ctx->getActiveFunction();
  if (give_expr.has_value()) {
    if (fun->getType()->asFunction()->getReturnType()->isReturnSelf()) {
      if (give_expr.value()->nodeType() != NodeType::SELF) {
        ctx->Error("This function is marked to return '' and cannot return anything else", fileRange);
      }
    }
    auto* retType = fun->getType()->asFunction()->getReturnType()->getType();
    if (give_expr.value()->hasTypeInferrance()) {
      give_expr.value()->asTypeInferrable()->setInferenceType(retType);
    }
    auto* retVal = give_expr.value()->emit(ctx);
    SHOW("RetType: " << retType->toString() << "\nRetValType: " << retVal->getType()->toString())
    if (retType->isSame(retVal->getType())) {
      if (retType->isVoid()) {
        IR::destructorCaller(ctx, fun);
        IR::memberFunctionHandler(ctx, fun);
        return new IR::Value(ctx->builder.CreateRetVoid(), retType, false, IR::Nature::temporary);
      } else {
        if (retVal->isImplicitPointer()) {
          if (retType->isTriviallyCopyable() || retType->isTriviallyMovable()) {
            auto* loadRes = ctx->builder.CreateLoad(retType->getLLVMType(), retVal->getLLVM());
            if (!retType->isTriviallyCopyable()) {
              if (!retVal->isVariable()) {
                ctx->Error("This expression does not have variability, and hence cannot be trivially moved from",
                           give_expr.value()->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(retType->getLLVMType()), retVal->getLLVM());
            }
            IR::destructorCaller(ctx, fun);
            IR::memberFunctionHandler(ctx, fun);
            return new IR::Value(ctx->builder.CreateRet(loadRes), retType, false, IR::Nature::temporary);
          } else {
            ctx->Error(
                "This expression is of type " + ctx->highlightError(retType->toString()) +
                    " which is not trivially copyable or movable. To convert the expression to a value, please use " +
                    ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                fileRange);
          }
        } else {
          IR::destructorCaller(ctx, fun);
          IR::memberFunctionHandler(ctx, fun);
          return new IR::Value(ctx->builder.CreateRet(retVal->getLLVM()), retType, false, IR::Nature::temporary);
        }
      }
    } else if (retType->isReference() &&
               retType->asReference()->getSubType()->isSame(
                   retVal->isReference() ? retVal->getType()->asReference()->getSubType() : retVal->getType()) &&
               (retType->asReference()->isSubtypeVariable()
                    ? (retVal->isImplicitPointer()
                           ? retVal->isVariable()
                           : (retVal->isReference() && retVal->getType()->asReference()->isSubtypeVariable()))
                    : true)) {
      SHOW("Return type is compatible")
      if (retType->isReference() && !retVal->isReference() && retVal->isLocalToFn()) {
        ctx->Warning(
            "Returning reference to a local value. The value pointed to by the reference might be destroyed before the function call is complete",
            fileRange);
      }
      if (retVal->isReference()) {
        retVal->loadImplicitPointer(ctx->builder);
      }
      IR::destructorCaller(ctx, fun);
      IR::memberFunctionHandler(ctx, fun);
      return new IR::Value(ctx->builder.CreateRet(retVal->getLLVM()), retType, false, IR::Nature::pure);
    } else if (retVal->getType()->isReference() && retVal->getType()->asReference()->getSubType()->isSame(retType)) {
      if (retType->isTriviallyCopyable() || retType->isTriviallyMovable()) {
        retVal->loadImplicitPointer(ctx->builder);
        auto* loadRes = ctx->builder.CreateLoad(retType->getLLVMType(), retVal->getLLVM());
        if (!retType->isTriviallyCopyable()) {
          if (!retVal->getType()->asReference()->isSubtypeVariable()) {
            ctx->Error("The expression is of type " + ctx->highlightError(retVal->getType()->toString()) +
                           " which is a reference without variability, and hence cannot be trivially moved from",
                       give_expr.value()->fileRange);
          }
          ctx->builder.CreateStore(llvm::Constant::getNullValue(retType->getLLVMType()), retVal->getLLVM());
        }
        IR::destructorCaller(ctx, fun);
        IR::memberFunctionHandler(ctx, fun);
        return new IR::Value(ctx->builder.CreateRet(loadRes), retType, false, IR::Nature::temporary);
      } else {
        ctx->Error(
            "This expression is of type " + ctx->highlightError(retType->toString()) +
                " which is not trivially copyable or movable. To convert the expression to a value, please use " +
                ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
            fileRange);
      }
    } else {
      ctx->Error("Given value type of the function is " + fun->getType()->asFunction()->getReturnType()->toString() +
                     ", but the provided value in the give sentence is " + retVal->getType()->toString(),
                 give_expr.value()->fileRange);
    }
  } else {
    if (fun->getType()->asFunction()->getReturnType()->getType()->isVoid()) {
      IR::destructorCaller(ctx, fun);
      IR::memberFunctionHandler(ctx, fun);
      return new IR::Value(ctx->builder.CreateRetVoid(), IR::VoidType::get(ctx->llctx), false, IR::Nature::temporary);
    } else {
      ctx->Error("No value is provided to give while the given type of the function is " +
                     ctx->highlightError(fun->getType()->asFunction()->getReturnType()->toString()) +
                     ". Please provide a value of the appropriate type",
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