#include "./ternary.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/maybe.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "./default.hpp"
#include "./none.hpp"

namespace qat::ast {

TernaryExpression::TernaryExpression(Expression* _condition, Expression* _trueExpr, Expression* _falseExpr,
                                     utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), condition(_condition), trueExpr(_trueExpr), falseExpr(_falseExpr) {}

IR::Value* TernaryExpression::emit(IR::Context* ctx) {
  auto* genCond = condition->emit(ctx);
  if (genCond) {
    auto* genType = genCond->getType();
    if (genType->isReference()) {
      genCond->loadImplicitPointer(ctx->builder);
      genType = genType->asReference()->getSubType();
    }
    if (genType->isInteger() || genType->isUnsignedInteger()) {
      auto* fun        = ctx->fn;
      auto* trueBlock  = new IR::Block(fun, fun->getBlock());
      auto* falseBlock = new IR::Block(fun, fun->getBlock());
      auto* mergeBlock = new IR::Block(fun, fun->getBlock());
      if (genCond->isReference() || genCond->isImplicitPointer()) {
        genCond = new IR::Value(ctx->builder.CreateLoad(genType->getLLVMType(), genCond->getLLVM()), genType, false,
                                IR::Nature::temporary);
      }
      ctx->builder.CreateCondBr(genCond->getLLVM(), trueBlock->getBB(), falseBlock->getBB());
      IR::Value* trueVal     = nullptr;
      IR::Value* falseVal    = nullptr;
      auto       trueNodeTy  = trueExpr->nodeType();
      auto       falseNodeTy = falseExpr->nodeType();
      if (trueNodeTy == NodeType::integerLiteral) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
        ((IntegerLiteral*)trueExpr)
            ->setType(falseVal->isReference() ? falseVal->getType()->asReference()->getSubType() : falseVal->getType());
      } else if (trueNodeTy == NodeType::unsignedLiteral) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
        ((UnsignedLiteral*)trueExpr)
            ->setType(falseVal->isReference() ? falseVal->getType()->asReference()->getSubType() : falseVal->getType());
      } else if (trueNodeTy == NodeType::nullPointer) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
        if (falseVal->isPointer() ||
            (falseVal->isReference() && falseVal->getType()->asReference()->getSubType()->isPointer())) {
          ((NullPointer*)trueExpr)
              ->setType(falseVal->isReference() ? falseVal->getType()->asReference()->getSubType()->asPointer()
                                                : falseVal->getType()->asPointer());
        }
      } else if (trueNodeTy == NodeType::none) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
        if (falseVal->getType()->isReference() ? falseVal->getType()->asReference()->getSubType()->isMaybe()
                                               : falseVal->getType()->isMaybe()) {
          ((NoneExpression*)trueExpr)
              ->setType(falseVal->isReference()
                            ? falseVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()
                            : falseVal->getType()->asMaybe()->getSubType());
        }
      } else if (trueNodeTy == NodeType::Default) {
        falseBlock->setActive(ctx->builder);
        falseVal = falseExpr->emit(ctx);
        ((Default*)trueExpr)
            ->setType(falseVal->isReference() ? falseVal->getType()->asReference()->getSubType() : falseVal->getType());
      } else if (falseNodeTy == NodeType::integerLiteral) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        ((IntegerLiteral*)falseExpr)
            ->setType(trueVal->isReference() ? trueVal->getType()->asReference()->getSubType() : trueVal->getType());
      } else if (falseNodeTy == NodeType::unsignedLiteral) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        ((UnsignedLiteral*)falseExpr)
            ->setType(trueVal->isReference() ? trueVal->getType()->asReference()->getSubType() : trueVal->getType());
      } else if (falseNodeTy == NodeType::nullPointer) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        if (trueVal->isPointer() ||
            (trueVal->isReference() && trueVal->getType()->asReference()->getSubType()->isPointer())) {
          ((NullPointer*)falseExpr)
              ->setType(trueVal->isReference() ? trueVal->getType()->asReference()->getSubType()->asPointer()
                                               : trueVal->getType()->asPointer());
        }
      } else if (falseNodeTy == NodeType::none) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        if (trueVal->getType()->isReference() ? trueVal->getType()->asReference()->getSubType()->isMaybe()
                                              : trueVal->getType()->isMaybe()) {
          ((NoneExpression*)falseExpr)
              ->setType(trueVal->isReference()
                            ? trueVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()
                            : trueVal->getType()->asMaybe()->getSubType());
        }
      } else if (falseNodeTy == NodeType::Default) {
        trueBlock->setActive(ctx->builder);
        trueVal = trueExpr->emit(ctx);
        ((Default*)falseExpr)
            ->setType(trueVal->isReference() ? trueVal->getType()->asReference()->getSubType() : trueVal->getType());
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

Json TernaryExpression::toJson() const {
  return Json()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJson())
      ._("trueCase", trueExpr->toJson())
      ._("falseCase", falseExpr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast