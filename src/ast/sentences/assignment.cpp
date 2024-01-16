#include "./assignment.hpp"
#include "../../IR/function.hpp"
#include "../expressions/copy.hpp"
#include "../expressions/move.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

#define MAX_RESPONSIVE_BITWIDTH 64u

IR::Value* Assignment::emit(IR::Context* ctx) {
  auto* lhsVal = lhs->emit(ctx);
  if (value->hasTypeInferrance()) {
    value->asTypeInferrable()->setInferenceType(lhsVal->getType());
  }
  SHOW("Emitted lhs of Assignment")
  if (lhsVal->isVariable() ||
      (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->isSubtypeVariable())) {
    SHOW("Is variable nature")
    if (lhsVal->getType()->isReference() || lhsVal->isImplicitPointer()) {
      if (lhsVal->isReference()) {
        lhsVal->loadImplicitPointer(ctx->builder);
      }
      if (value->nodeType() == NodeType::COPY) {
        auto copyExp          = (ast::Copy*)value;
        copyExp->isAssignment = true;
        copyExp->setCreateIn(lhsVal);
        (void)value->emit(ctx);
        return nullptr;
      } else if (value->nodeType() == NodeType::MOVE_EXPRESSION) {
        auto moveExp          = (ast::Move*)value;
        moveExp->isAssignment = true;
        moveExp->setCreateIn(lhsVal);
        (void)value->emit(ctx);
        return nullptr;
      }
      auto* expVal = value->emit(ctx);
      SHOW("Getting IR types")
      auto* lhsTy = lhsVal->getType();
      auto* expTy = expVal->getType();
      if (lhsTy->isSame(expTy) || lhsTy->isCompatible(expTy) ||
          (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isSame(
                                       expTy->isReference() ? expTy->asReference()->getSubType() : expTy)) ||
          (expTy->isReference() && expTy->asReference()->getSubType()->isSame(
                                       lhsTy->isReference() ? lhsTy->asReference()->getSubType() : lhsTy))) {
        SHOW("The general types are the same")
        if (lhsVal->isImplicitPointer() && lhsTy->isReference()) {
          SHOW("LHS is implicit pointer")
          lhsVal->loadImplicitPointer(ctx->builder);
          SHOW("Loaded implicit pointer")
        }
        if (expTy->isReference() || expVal->isImplicitPointer()) {
          if (expTy->isReference()) {
            expVal->loadImplicitPointer(ctx->builder);
            SHOW("Expression for assignment is of type " << expTy->asReference()->getSubType()->toString())
            expTy = expTy->asReference()->getSubType();
          }
          if (expTy->isTriviallyCopyable() || expTy->isTriviallyMovable()) {
            auto prevRef = expVal->getLLVM();
            expVal = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expVal->getLLVM()), expVal->getType(),
                                   expVal->isVariable(), expVal->getNature());
            if (!expTy->isTriviallyCopyable()) {
              if (expTy->isReference() && !expTy->asReference()->isSubtypeVariable()) {
                ctx->Error("This expression is of type " + ctx->highlightError(expTy->toString()) +
                               " which is a reference without variability and hence cannot be trivially moved from",
                           value->fileRange);
              } else if (!expVal->isVariable()) {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           fileRange);
              }
              // FIXME - Use zero/default value
              ctx->builder.CreateStore(llvm::ConstantExpr::getNullValue(expTy->getLLVMType()), prevRef);
            }
            SHOW("Creating store with LHS type: " << lhsVal->getType()->toString()
                                                  << " and RHS type: " << expVal->getType()->toString())
            ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
          } else {
            ctx->Error("The expression on the right hand side is of type " + ctx->highlightError(expTy->toString()) +
                           " which is not trivially copyable or trivially movable. Please use " +
                           ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                       value->fileRange);
          }
        } else {
          ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
        }
        return nullptr;
      } else {
        ctx->Error("Type of the left hand side of the assignment is " + ctx->highlightError(lhsTy->toString()) +
                       " and the type of right hand side is " + ctx->highlightError(expTy->toString()) +
                       ". The types of both sides are not compatible for assignment. Please check the logic",
                   fileRange);
      }
    } else {
      ctx->Error("Left hand side of the assignment cannot be assigned to as it is value", lhs->fileRange);
    }
  } else {
    if (lhsVal->getType()->isReference()) {
      ctx->Error(
          "Left hand side of the assignment cannot be assigned to because the reference does not have variability",
          lhs->fileRange);
    } else if (lhsVal->getType()->isPointer()) {
      ctx->Error(
          "Left hand side of the assignment cannot be assigned to because it is of pointer type. If you intend to change the "
          "value pointed to by this pointer, consider dereferencing it before assigning",
          lhs->fileRange);
    } else {
      ctx->Error("Left hand side of the assignment cannot be assigned to because it does not have variability",
                 lhs->fileRange);
    }
  }
  return nullptr;
}

Json Assignment::toJson() const {
  return Json()._("nodeType", "assignment")._("lhs", lhs->toJson())._("rhs", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast
