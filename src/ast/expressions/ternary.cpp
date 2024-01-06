#include "./ternary.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

TernaryExpression::TernaryExpression(Expression* _condition, Expression* _trueExpr, Expression* _falseExpr,
                                     FileRange _fileRange)
    : Expression(std::move(_fileRange)), condition(_condition), trueExpr(_trueExpr), falseExpr(_falseExpr) {}

IR::Value* TernaryExpression::emit(IR::Context* ctx) {
  auto* genCond = condition->emit(ctx);
  if (genCond) {
    auto* genType = genCond->getType();
    if (genType->isReference()) {
      genCond->loadImplicitPointer(ctx->builder);
      genType = genType->asReference()->getSubType();
    }
    // FIXME - Support bool conversions and maybe
    if (genType->isBool()) {
      auto* fun        = ctx->getActiveFunction();
      auto* trueBlock  = new IR::Block(fun, fun->getBlock());
      auto* falseBlock = new IR::Block(fun, fun->getBlock());
      auto* mergeBlock = new IR::Block(fun, fun->getBlock());
      if (genCond->isReference() || genCond->isImplicitPointer()) {
        genCond = new IR::Value(ctx->builder.CreateLoad(genType->getLLVMType(), genCond->getLLVM()), genType, false,
                                IR::Nature::temporary);
      }
      ctx->builder.CreateCondBr(genCond->getLLVM(), trueBlock->getBB(), falseBlock->getBB());
      IR::Value* trueVal  = nullptr;
      IR::Value* falseVal = nullptr;
      if (isTypeInferred()) {
        if (trueExpr->hasTypeInferrance()) {
          trueExpr->asTypeInferrable()->setInferenceType(inferredType);
        }
        if (falseExpr->hasTypeInferrance()) {
          falseExpr->asTypeInferrable()->setInferenceType(inferredType);
        }
      } else {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        if (falseExpr->hasTypeInferrance()) {
          falseExpr->asTypeInferrable()->setInferenceType(trueVal->getType());
        }
      }
      /* true case */
      if (!trueVal) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
      }
      /* false case */
      if (!falseVal) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
      }

      /* rest block */
      llvm::PHINode* phiNode = nullptr;
      if (trueVal && falseVal) {
        auto* trueType  = trueVal->getType();
        auto* falseType = falseVal->getType();
        if (trueType->isSame(falseType) ||
            (trueType->isReference() && trueType->asReference()->getSubType()->isSame(falseType)) ||
            (falseType->isReference() && falseType->asReference()->getSubType()->isSame(trueType))) {
          IR::QatType* resultType = nullptr;
          if ((trueType->isReference() || trueVal->isImplicitPointer()) &&
              (!falseType->isReference() && !falseVal->isImplicitPointer())) {
            trueBlock->setActive(ctx->builder);
            trueVal->loadImplicitPointer(ctx->builder);
            if (trueType->isReference()) {
              resultType = trueType->asReference()->getSubType();
              trueVal    = new IR::Value(
                  ctx->builder.CreateLoad(trueType->asReference()->getSubType()->getLLVMType(), trueVal->getLLVM()),
                  trueType->asReference()->getSubType(), false, IR::Nature::temporary);
            } else {
              resultType = trueType;
            }
          } else if ((falseType->isReference() || falseVal->isImplicitPointer()) &&
                     (!trueType->isReference() && !trueVal->isImplicitPointer())) {
            falseBlock->setActive(ctx->builder);
            falseVal->loadImplicitPointer(ctx->builder);
            if (falseType->isReference()) {
              resultType = falseType->asReference()->getSubType();
              falseVal   = new IR::Value(
                  ctx->builder.CreateLoad(falseType->asReference()->getSubType()->getLLVMType(), falseVal->getLLVM()),
                  falseType->asReference()->getSubType(), false, IR::Nature::temporary);
            } else {
              resultType = falseType;
            }
          } else {
            resultType = trueType;
          }
          trueBlock->setActive(ctx->builder);
          (void)IR::addBranch(ctx->builder, mergeBlock->getBB());
          falseBlock->setActive(ctx->builder);
          (void)IR::addBranch(ctx->builder, mergeBlock->getBB());
          mergeBlock->setActive(ctx->builder);
          SHOW("Creating PhiNode, resultType address: " << resultType)
          phiNode = ctx->builder.CreatePHI(resultType->getLLVMType(), 2);
          SHOW("Getting true value")
          phiNode->addIncoming(trueVal->getLLVM(), trueBlock->getBB());
          SHOW("Getting false value")
          phiNode->addIncoming(falseVal->getLLVM(), falseBlock->getBB());
          SHOW("Phinode type is: " << phiNode->getType()->getTypeID());
          return new IR::Value(phiNode, resultType, false, IR::Nature::temporary);
        } else {
          ctx->Error("Ternary expression is giving values of different types", fileRange);
        }
      } else {
        ctx->Error("Ternary " + ctx->highlightError((trueVal == nullptr) ? "if" : "else") +
                       " expression is not giving any value",
                   (trueVal == nullptr) ? trueExpr->fileRange : falseExpr->fileRange);
      }
    } else {
      ctx->Error("Condition expression is of the type " + ctx->highlightError(genCond->getType()->toString()) +
                     ", but ternary expression expects an expression of " + ctx->highlightError("bool") + " type",
                 trueExpr->fileRange);
    }
  } else {
    ctx->Error("Condition expression is null, but `if` sentence "
               "expects an expression of `bool` or `int<1>` type",
               falseExpr->fileRange);
  }
  return nullptr;
}

Json TernaryExpression::toJson() const {
  return Json()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJson())
      ._("trueCase", trueExpr->toJson())
      ._("falseCase", falseExpr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast