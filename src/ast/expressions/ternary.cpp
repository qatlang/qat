#include "./ternary.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

TernaryExpression::TernaryExpression(Expression      *_condition,
                                     Expression      *_trueExpr,
                                     Expression      *_falseExpr,
                                     utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), condition(_condition),
      trueExpr(_trueExpr), falseExpr(_falseExpr) {}

IR::Value *TernaryExpression::emit(IR::Context *ctx) {
  auto *genCond = condition->emit(ctx);
  if (genCond) {
    if (genCond->getType()->isInteger() ||
        genCond->getType()->isUnsignedInteger()) {
      auto *fun        = ctx->fn;
      auto *trueBlock  = new IR::Block(fun, fun->getBlock());
      auto *falseBlock = new IR::Block(fun, fun->getBlock());
      auto *mergeBlock = new IR::Block(fun, fun->getBlock());
      ctx->builder.CreateCondBr(genCond->getLLVM(), trueBlock->getBB(),
                                falseBlock->getBB());

      /* true case */
      trueBlock->setActive(ctx->builder);
      auto *trueVal = trueExpr->emit(ctx);
      (void)IR::addBranch(ctx->builder, mergeBlock->getBB());

      /* false case */
      falseBlock->setActive(ctx->builder);
      auto *falseVal = falseExpr->emit(ctx);
      (void)IR::addBranch(ctx->builder, mergeBlock->getBB());

      /* rest block */
      llvm::PHINode *phiNode = nullptr;
      if (trueVal && falseVal) {
        auto *trueType  = trueVal->getType();
        auto *falseType = falseVal->getType();
        if (trueType->isSame(falseType) ||
            (trueType->isReference() &&
             trueType->asReference()->getSubType()->isSame(falseType)) ||
            (falseType->isReference() &&
             falseType->asReference()->getSubType()->isSame(trueType))) {
          IR::QatType *resultType = nullptr;
          if (trueType->isReference() && !falseType->isReference()) {
            resultType = trueType->asReference()->getSubType();
            trueBlock->setActive(ctx->builder);
            trueVal->loadImplicitPointer(ctx->builder);
            trueVal = new IR::Value(
                ctx->builder.CreateLoad(
                    trueType->asReference()->getSubType()->getLLVMType(),
                    trueVal->getLLVM()),
                trueType->asReference()->getSubType(), false,
                IR::Nature::temporary);
          } else if (falseType->isReference() && !trueType->isReference()) {
            resultType = falseType->asReference()->getSubType();
            falseBlock->setActive(ctx->builder);
            falseVal->loadImplicitPointer(ctx->builder);
            falseVal = new IR::Value(
                ctx->builder.CreateLoad(
                    falseType->asReference()->getSubType()->getLLVMType(),
                    falseVal->getLLVM()),
                falseType->asReference()->getSubType(), false,
                IR::Nature::temporary);
          } else {
            resultType = trueType;
          }
          mergeBlock->setActive(ctx->builder);
          phiNode = ctx->builder.CreatePHI(trueType->getLLVMType(), 2,
                                           utils::unique_id());
          phiNode->addIncoming(trueVal->getLLVM(), trueBlock->getBB());
          phiNode->addIncoming(falseVal->getLLVM(), falseBlock->getBB());
          return new IR::Value(phiNode, resultType, false,
                               IR::Nature::temporary);
        } else {
          ctx->Error("Ternary expression is giving values of different types",
                     fileRange);
        }
      } else {
        ctx->Error(
            "Ternary " +
                ctx->highlightError((trueVal == nullptr) ? "if" : "else") +
                " expression is not giving any value",
            (trueVal == nullptr) ? trueExpr->fileRange : falseExpr->fileRange);
      }
    } else {
      ctx->Error("Condition expression is of the type " +
                     ctx->highlightError(genCond->getType()->toString()) +
                     ", but ternary expression expects an expression of signed "
                     "or unsigned integer type",
                 trueExpr->fileRange);
    }
  } else {
    ctx->Error("Condition expression is null, but `if` sentence "
               "expects an expression of `bool` or `int<1>` type",
               falseExpr->fileRange);
  }
  return nullptr;
}

nuo::Json TernaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJson())
      ._("trueCase", trueExpr->toJson())
      ._("falseCase", falseExpr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast